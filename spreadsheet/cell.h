#pragma once

#include "common.h"
#include "formula.h"
#include "sheet.h"

class Sheet;

class Cell : public CellInterface 
{
public:
    Cell(const SheetInterface& sheet);
    ~Cell();

    void Set(std::string text);
    void SetPos(Position pos);
    void SetRef(bool state) const;
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    bool IsReferenced() const;

private:

    class Impl
    {
        public:
        
        virtual Value GetValue() const
        {
            return "";
        }
        
        virtual std::string GetText() const
        {
            return "";
        }
    };
    
    class EmptyImpl : public Impl
    {
        public:
        
        EmptyImpl() = default;
        
        Value GetValue() const override
        {
            return "";
        }
        
        std::string GetText() const override
        {
            return "";
        }
    };
    
    class TextImpl : public Impl
    {
        public:
        
        TextImpl() = default;
        
        TextImpl(std::string& text)
        {
            value_ = std::move(text);
        }
                
        void SetValue(std::string& text)
        {
            value_ = std::move(text);
        }
        
        Value GetValue() const override
        {
            if(value_[0] == '\'')
            {
                std::string tmp(value_.begin() + 1, value_.end());
                return tmp;
            }
            
            return value_;
        }
        
        std::string GetText() const override
        {
            return value_;
        }
        
        private:
        std::string value_;
    };
    
    class FormulaImpl : public Impl
    {
        public:
        
        FormulaImpl(const SheetInterface& sheet)
        :sheet_(sheet){}
        
        FormulaImpl(const SheetInterface& sheet, std::string& text)
        :sheet_(sheet){
            value_ = std::move(text);
        }
                
        void SetValue(std::string& text)
        {
            value_ = std::move(text);
        }
        
        Value GetValue() const override
        {
            std::string tmp(value_.begin() + 1, value_.end());
            
            if(tmp.size() == 0)
                return value_;
            
            std::unique_ptr<FormulaInterface> formula = ParseFormula(tmp);
            std::variant<double, FormulaError> result = formula->Evaluate(sheet_);
            
            if(std::holds_alternative<double>(result))
            {
                return std::get<double>(result);
            }
            else
            {
                return std::get<FormulaError>(result);
            }
            
        }
        
        std::string GetText() const override
        {
            return value_;
        }
        
        private:
        std::string value_;
        const SheetInterface& sheet_;
    };
    
    std::unique_ptr<Impl> impl_;
    
    mutable bool is_referenced_ = false;
    
    const SheetInterface& sheet_;
    Position current_pos_ = {-1, -1};
};