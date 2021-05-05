#include "LR.hpp"

namespace Parser
{
    LR::LR(const Grammar &grammar)
    : mGrammar(grammar)
    {
        mValid = false;
    }

    bool LR::valid() const
    {
        return mValid;
    }

    const Grammar &LR::grammar() const
    {
        return mGrammar;
    }

    const LR::Conflict &LR::conflict() const
    {
        return mConflict;
    }

    unsigned int LR::symbolIndex(const Grammar::Symbol &symbol) const
    {
        switch(symbol.type) {
            case Grammar::Symbol::Type::Terminal:
                return terminalIndex(symbol.index);
            case Grammar::Symbol::Type::Nonterminal:
                return ruleIndex(symbol.index);
            default:
                return UINT_MAX;
        }
    }

    unsigned int LR::terminalIndex(unsigned int terminal) const
    {
        return terminal;
    }

    unsigned int LR::ruleIndex(unsigned int rule) const
    {
        return (unsigned int)mGrammar.terminals().size() + rule;
    }

}