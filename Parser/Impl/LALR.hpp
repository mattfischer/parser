#ifndef PARSER_IMPL_LALR_HPP
#define PARSER_IMPL_LALR_HPP

#include "Parser/Impl/LR.hpp"

namespace Parser::Impl
{
    template<typename ParseData> class LALR : public LR<ParseData>
    {
    public:
        LALR(const Grammar &grammar)
        : LR<ParseData>(grammar, std::make_unique<LRTable::LALR>(grammar))
        {
        }
    };
}
#endif