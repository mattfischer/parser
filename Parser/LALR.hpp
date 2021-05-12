#ifndef PARSER_LALR_HPP
#define PARSER_LALR_HPP

#include "Parser/LRSingle.hpp"

namespace Parser
{
    class LALR : public LRSingle
    {
    public:
        LALR(const Grammar &grammar);
    };
}
#endif