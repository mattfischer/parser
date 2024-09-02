#include "Parser/Impl/SLR.hpp"

namespace Parser::Impl
{
    SLR::SLR(const Grammar &grammar)
    : LR(grammar, std::make_unique<LRTable::SLR>(grammar))
    {
    }
}