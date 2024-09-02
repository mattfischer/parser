#ifndef PARSER_IMPL_LALR_HPP
#define PARSER_IMPL_LALR_HPP

#include "Parser/Impl/LR.hpp"

namespace Parser::Impl
{
    class LALR : public LR
    {
    public:
        LALR(const Grammar &grammar);
    };
}
#endif