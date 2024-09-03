#include "Parser/Impl/LRTable/SLR.hpp"

namespace Parser::Impl::LRTable {
    SLR::SLR(const Grammar &grammar)
    : Single(grammar)
    {
        std::vector<State> states = computeStates();

        std::vector<std::set<unsigned int>> firstSets;
        std::set<unsigned int> nullableNonterminals;
        Base::grammar().computeSets(firstSets, mFollowSets, nullableNonterminals);

        computeParseTable(states);
    }

    const std::set<unsigned int> &SLR::getReduceLookahead(unsigned int state, unsigned int rule) const
    {
        return mFollowSets[rule];
    }
}