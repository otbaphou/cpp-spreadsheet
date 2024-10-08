#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() 
{}

void CheckPos(Position pos)
{
    if(!pos.IsValid())
    {
        throw InvalidPositionException("Invalid Position!");
    }
}

void Sheet::SetCell(Position pos, std::string text) 
{
    CheckPos(pos);
    
    if(pos.row >= height)
    {
        height = pos.row + 1;
    }
    if(pos.col >= width){
        width = pos.col + 1;
    }
    
    std::unique_ptr<Cell>& cell_ptr = data_[pos.row][pos.col];
    
    if(cell_ptr == nullptr)
    {
        cell_ptr = std::unique_ptr<Cell>(new Cell{*this});
    }
    
    cell_ptr->SetPos(pos);
    
    std::string tmp = cell_ptr->GetText();
    
    cell_ptr->Set(text);
    
    if(HasCyclicDependency(pos))
    {
        cell_ptr->Set(tmp);
        throw CircularDependencyException("Cyclic dependency detected!");
    }
}

const CellInterface* Sheet::GetCell(Position pos) const 
{
    CheckPos(pos);
    
    const std::map<int, std::map<int, std::unique_ptr<Cell>>>::const_iterator row_value = data_.find(pos.row);
    
    if(row_value != data_.end())
    {
        if((*row_value).second.find(pos.col) != (*row_value).second.end())
        {    
            return (*row_value).second.at(pos.col).get();
        }
    }
    
    return nullptr;
}

CellInterface* Sheet::GetCell(Position pos) 
{
    CheckPos(pos);
    
    if(pos.row < height && pos.col < width)
    {
        return data_[pos.row][pos.col].get();
    }
    
    return nullptr;
}

void Sheet::ClearCell(Position pos) 
{
    CheckPos(pos);
    
    if(pos.row >= height || pos.col >= width)
    {
        return;
    }
        
    if(GetCell({pos.row, pos.col}) != nullptr)
    {
        cache_.erase(pos);
        dependencies_.erase(pos);
        data_[pos.row][pos.col]->Clear();
    }
    
    data_[pos.row].erase(pos.col);
    
    if(data_[pos.row].size() == 0)
    {
        data_.erase(pos.row);
    }
    
    int new_height = data_.empty() ? 0 : data_.rbegin()->first + 1;
    int new_width = 0;
    
    for(const auto& row : data_)
    {
        int tmp_width = row.second.rbegin()->first;
        
        if(tmp_width >= new_width)
            new_width = tmp_width + 1;
    }
    
    height = new_height;
    width = new_width;
}

Size Sheet::GetPrintableSize() const 
{
    return {height, width};
}

void Sheet::PrintValues(std::ostream& output) const 
{
    int offset = width - 1;
    for(int row = 0; row < height; ++row)
    {
        for(int col = 0; col < width; ++col)
        {
            if(GetCell({row, col}) != nullptr)
            {
                auto result = GetCell({row, col})->GetValue();
                
                if(std::holds_alternative<double>(result))
                {
                    output << std::get<double>(result);
                }
                
                if(std::holds_alternative<std::string>(result))
                {
                    output << std::get<std::string>(result);
                }
                
                if(std::holds_alternative<FormulaError>(result))
                {
                    output << std::get<FormulaError>(result);
                }
            }            
            if(col < offset)
            {
                output << "\t";
            }
        }
        output << "\n";
    }
}

const Cell* Sheet::GetConcreteCell(Position pos) const
{
    CheckPos(pos);
        
    const std::map<int, std::map<int, std::unique_ptr<Cell>>>::const_iterator row_value = data_.find(pos.row);

    if(row_value != data_.end())
    {
        if((*row_value).second.find(pos.col) != (*row_value).second.end())
        {    
            return (*row_value).second.at(pos.col).get();
        }
    }
    
    return nullptr;
}

Cell* Sheet::GetConcreteCell(Position pos)
{
    CheckPos(pos);
    
    if(pos.row < height && pos.col < width)
    {    
        return data_[pos.row][pos.col].get();
    }
    
    return nullptr;
}

std::variant<std::string, double, FormulaError> Sheet::GetCachedValue(Position pos) const
{   
    return cache_[pos];
}

std::vector<Position> Sheet::GetReferencedPositions(Position pos) const
{
    return dependencies_[pos];
}

Size Sheet::GetActualSize() const
{
    return {height, width};
}

void Sheet::MaybeIncreaseSizeToIncludePosition(Position pos)
{
    if(pos.row > height)
    {
        height = pos.row + 1;
    }
    
    if(pos.col > width)
    {
        width = pos.col + 1;
    }
}

void Sheet::PrintTexts(std::ostream& output) const 
{
    int offset = width - 1;
    for(int row = 0; row < height; ++row)
    {
        for(int col = 0; col < width; ++col)
        {
            if(GetCell({row, col}) != nullptr)
            {
                output << GetCell({row, col})->GetText();
            }
            if(col < offset)
            {
            output << "\t";
            }
        }
        output << "\n";
    }
}    

void Sheet::StoreCache(Position pos, CachedValue val) const
{
    cache_[pos] = std::move(val);
}
    
void Sheet::StoreRefs(Position pos, std::vector<Position> refs) const
{
    if(refs.size() > 0)
    {
        Sheet::GetConcreteCell(pos)->SetRef(true);
    }
    
    dependencies_[pos] = std::move(refs);
}
using RecMap = std::unordered_map<Position, bool, PositionHasher>;
bool Sheet::Cycle(Position start_pos, Position element, RecMap visited, RecMap elements) const
{
    if(visited[element])
    {
        elements[element] = false;
        return false;
    }
    
    visited[element] = true;
    elements[element] = true;
    
    for(Position neighboring_element : dependencies_[element])
    {
        if(!visited[neighboring_element] && Cycle(start_pos, neighboring_element, visited, elements))
        {
            return true;
        }
        else
        {
            if(elements[neighboring_element])
            {
                return true;
            }
        }
    }
    
    return false;
}

bool Sheet::HasCyclicDependency(Position pos) const
{
    RecMap visited;
    RecMap elements;
    std::vector<Position> refs = std::move(GetReferencedPositions(pos));
    
    if(refs.size() == 0)
        return false;
    
    for(const auto& entry : dependencies_)
    {
        if(Cycle(pos, entry.first, visited, elements))
        {
            return true;
        }
    }
    
    return false;
}

std::unique_ptr<SheetInterface> CreateSheet() 
{
    return std::make_unique<Sheet>();
}



