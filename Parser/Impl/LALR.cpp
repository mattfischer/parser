#include "Parser/Impl/LALR.hpp"

#include <iostream>
#include <sstream>

namespace Parser::Impl
{
    LALR::LALR(const Grammar &grammar)
    : LR(grammar, std::make_unique<LRTable::LALR>(grammar))
    {
    }
}