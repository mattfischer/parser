#ifndef PARSER_IMPL_LALR_HPP
#define PARSER_IMPL_LALR_HPP

#include "Parser/Impl/LR.hpp"
#include "Parser/Impl/LRTable/LALR.hpp"

namespace Parser::Impl
{
    template<typename ParseData> using LALR = LR<ParseData, LRTable::LALR>;
}
#endif