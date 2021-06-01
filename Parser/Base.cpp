#include "Parser/Base.hpp"

namespace Parser
{
    Base::Base(const Grammar &grammar)
    : mGrammar(grammar)
    {
    }

    const Grammar &Base::grammar() const
    {
        return mGrammar;
    }
}