#ifndef PARSER_BASE_HPP
#define PARSER_BASE_HPP

#include "Parser/Grammar.hpp"

namespace Parser
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