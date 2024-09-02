#ifndef PARSER_IMPL_LRTABLE_SLR_HPP
#define PARSER_IMPL_LRTABLE_SLR_HPP

#include "Parser/Impl/LRTable/Single.hpp"

namespace Parser::Impl::LRTable {
    class SLR : public Single {
    public:
        SLR(const Grammar &grammar);
    };
}
#endif