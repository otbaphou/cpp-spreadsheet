#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <map>

class Cell;

struct PositionHasher 
{
    std::size_t operator()(const Position& position) const 
    {
        std::size_t hash;
        std::size_t h1 = std::hash<int>{}(position.col);
        std::size_t h2 = std::hash<int>{}(position.row);
        
        hash = h1 ^ (h2 << 1);
        return hash;
    }
};

class Sheet : public SheetInterface 
{    
public:
    using CachedValue = std::variant<std::string, double, FormulaError>;
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;
    
    CachedValue GetCachedValue(Position pos) const override;
    std::vector<Position> GetReferencedPositions(Position pos) const override;
    
    const Cell* GetConcreteCell(Position pos) const;
    Cell* GetConcreteCell(Position pos);
    
    void StoreCache(Position pos, CachedValue val) const;
    void StoreRefs(Position pos, std::vector<Position> refs) const override;
    
    using RecMap = std::unordered_map<Position, bool, PositionHasher>;
    bool Cycle(Position start_pos, Position element_pos, RecMap visited, RecMap elements) const;
    bool HasCyclicDependency(Position pos) const;

private:
    void MaybeIncreaseSizeToIncludePosition(Position pos);
    //void PrintCells(std::ostream& output, const std::function<void(const CellInterface&)>& printCell) const;
    Size GetActualSize() const;

    std::map<int, std::map<int, std::unique_ptr<Cell>>> data_;
    mutable std::unordered_map<Position, CachedValue, PositionHasher> cache_;
    mutable std::unordered_map<Position, std::vector<Position>, PositionHasher> dependencies_;
    
    int width = 0, height = 0;
};