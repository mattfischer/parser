#ifndef PARSER_IMPL_LRTABLE_LALR_HPP
#define PARSER_IMPL_LRTABLE_LALR_HPP

#include "Parser/Impl/LRTable/Single.hpp"

namespace Parser::Impl::LRTable {
    class LALR : public Single {
    public:
        LALR(const Grammar &grammar);

    protected:
        virtual const std::set<unsigned int> &getReduceLookahead(unsigned int state, unsigned int rule) const;

    private:
        std::map<std::pair<unsigned int, unsigned int>, std::set<unsigned int>> mFollowPerStateSets;  
    };
}
#endif