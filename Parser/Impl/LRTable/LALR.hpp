#ifndef PARSER_IMPL_LRTABLE_LALR_HPP
#define PARSER_IMPL_LRTABLE_LALR_HPP

#include "Parser/Impl/LRTable/Single.hpp"

namespace Parser::Impl::LRTable {
    class LALR : public Single {
    public:
        LALR(const Grammar &grammar);
    };
}
#endif