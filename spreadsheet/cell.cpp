#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

Cell::Cell(const SheetInterface& sheet)
:sheet_(sheet)
{
    impl_ = std::make_unique<Impl>(EmptyImpl());
}

Cell::~Cell() 
{
    impl_.release();
}

void Cell::Set(std::string text)
{
    if(text.empty())
    {
        impl_.release();
        impl_ = std::make_unique<Impl>(EmptyImpl());
    }
    
    is_referenced_ = false;
    
    if(text[0] == FORMULA_SIGN && !(text.size() == 1))
    {        
        std::string tmp(text.begin() + 1, text.end());
        std::unique_ptr<FormulaInterface> formula;
            
        try
        {
             formula = ParseFormula(tmp);
        }
        catch(FormulaException& e)
        {
            throw FormulaException("Error. Formula failed parsing!");
        }
        
        std::vector<Position> refs = std::move(formula->GetReferencedCells());
        
        sheet_.StoreRefs(current_pos_, refs);
        
        std::string expr = formula->GetExpression();
        expr = '=' + expr;
        
        impl_.release();
        impl_ = std::make_unique<FormulaImpl>(sheet_, expr);
    }
    else
    {
        impl_.release();
        impl_ = std::make_unique<TextImpl>(TextImpl(text));
    }
    
    sheet_.StoreCache(current_pos_, impl_->GetValue());
}
void Cell::SetPos(Position pos)
{
    current_pos_ = pos;
}

void Cell::SetRef(bool state = true) const
{
    is_referenced_ = state;
}

void Cell::Clear() 
{
    Set("");
}

Cell::Value Cell::GetValue() const 
{
    return impl_->GetValue();
}

std::string Cell::GetText() const 
{
    return impl_->GetText();
}

bool Cell::IsReferenced() const
{
    return is_referenced_;
}

std::vector<Position> Cell::GetReferencedCells() const
{
    if(is_referenced_)
    {
        return sheet_.GetReferencedPositions(current_pos_);
    }
    
    return {};
}