#ifndef PARSER_IMPL_LRTABLE_SLR_HPP
#define PARSER_IMPL_LRTABLE_SLR_HPP

#include "Parser/Impl/LRTable/Single.hpp"

namespace Parser::Impl::LRTable {
    class SLR : public Single {
    public:
        SLR(const Grammar &grammar);

    protected:
        virtual const std::set<unsigned int> &getReduceLookahead(unsigned int state, unsigned int rule) const;

    private:
        std::vector<std::set<unsigned int>> mFollowSets;
    };
}
#endif