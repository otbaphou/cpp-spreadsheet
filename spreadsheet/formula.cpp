#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <set>
#include <cmath>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}



namespace {
class Formula : public FormulaInterface 
{
    public:
        explicit Formula(std::string expression) try
        : ast_(ParseFormulaAST(expression)) 
        {
            ParseRefs();
        } 
        catch (const std::exception& exc) 
        {
            std::throw_with_nested(FormulaException(exc.what()));
        }
        
        Value Evaluate(const SheetInterface& sheet) const override
        {
            ParseRefs();
            
            for(auto& entry : refs_)
            {
                auto val = sheet.GetCachedValue(entry); 
                
                if(std::holds_alternative<FormulaError>(val))
                {
                    return std::get<FormulaError>(val);
                }
            }
            
            try
            {
                Value result = ast_.Execute(sheet);
                
                if(std::holds_alternative<double>(result))
                {
                    if(!std::isfinite(std::get<double>(result)))
                    {
                        return FormulaError::Category::Arithmetic;
                    }
                }
                
                return result;
            }
            catch ( const FormulaException& e )
            {
                
                switch(e.what()[1])
                {
                    case 'A':
                        return FormulaError::Category::Arithmetic;
                        break;
                    
                    case 'R':
                        return FormulaError::Category::Ref;
                        break;
                    
                    case 'V':
                        return FormulaError::Category::Value;
                        break;
                        
                    default:
                        return FormulaError::Category::Ref;
                        break;
                }
                
            }
            catch ( const FormulaError& e )
            {
                //std::cout << "Handled FormulaError!\n";
                return e;
            }
            catch(...)
            {
                //std::cerr << "Unknown exception: \n";// << e.what() << std::endl;
                return FormulaError::Category::Arithmetic;
            }

        }
        
        std::string GetExpression() const override 
        {
            std::ostringstream ss;
            ast_.PrintFormula(ss);
            return ss.str();
        }
    
        std::vector<Position> GetReferencedCells() const
        {
            return refs_;
        }
        
    private:
    
        void ParseRefs() const
        {
            std::set<Position> checker;
            for(Position p : ast_.GetCells())
            {
                if(checker.find(p) == checker.end())
                {
                    checker.insert(p);
                    refs_.push_back(p);
                }
            }
        }
        
        FormulaAST ast_;
        mutable std::vector<Position> refs_;
    };
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) 
{
    return std::make_unique<Formula>(std::move(expression));
}

