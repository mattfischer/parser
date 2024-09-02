#include "Parser/Impl/Base.hpp"

namespace Parser::Impl
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