#ifndef PARSER_IMPL_SLR_HPP
#define PARSER_IMPL_SLR_HPP

#include "Parser/Impl/LR.hpp"

namespace Parser::Impl
{
    template<typename ParseData> using SLR = LR<ParseData, LRTable::SLR>;
}
#endif