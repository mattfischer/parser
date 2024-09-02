#ifndef PARSER_IMPL_SLR_HPP
#define PARSER_IMPL_SLR_HPP

#include "Parser/Impl/LR.hpp"

#include <set>
#include <map>
#include <vector>

namespace Parser::Impl
{
    template<typename ParseData> class SLR : public LR<ParseData>
    {
    public:
        SLR(const Grammar &grammar)
        : LR<ParseData>(grammar, std::make_unique<LRTable::SLR>(grammar))
        {
        }
    };
}
#endif