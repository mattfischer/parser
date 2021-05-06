#ifndef PARSER_LALR_HPP
#define PARSER_LALR_HPP

#include "Parser/LR.hpp"

namespace Parser
{
    class LALR : public LR
    {
    public:
        LALR(const Grammar &grammar);
    };
}
#endif