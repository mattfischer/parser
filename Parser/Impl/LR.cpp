#include "Parser/Impl/LR.hpp"

namespace Parser::Impl
{
    LR::LR(const Grammar &grammar, std::unique_ptr<LRTable::Single> parseTable)
    : Base(grammar)
    , mParseTable(std::move(parseTable))
    {
    }
}