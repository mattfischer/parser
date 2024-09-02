#ifndef PARSER_IMPL_BASE_HPP
#define PARSER_IMPL_BASE_HPP

#include "Parser/Grammar.hpp"

namespace Parser::Impl
{
    class Base
    {
    public:
        Base(const Grammar &grammar);
    
        const Grammar &grammar() const;

    protected:
        const Grammar &mGrammar;
    };
}
#endif