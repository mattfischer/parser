#ifndef PARSER_IMPL_SLR_HPP
#define PARSER_IMPL_SLR_HPP

#include "Parser/Impl/LR.hpp"

#include <set>
#include <map>
#include <vector>

namespace Parser::Impl
{
    class SLR : public LR
    {
    public:
        SLR(const Grammar &grammar);

    };
}
#endif