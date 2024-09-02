#include "Parser/Impl/LRTable/SLR.hpp"

namespace Parser::Impl::LRTable {
    SLR::SLR(const Grammar &grammar)
    : Single(grammar)
    {
        std::vector<State> states = computeStates();

        std::vector<std::set<unsigned int>> firstSets;
        std::vector<std::set<unsigned int>> followSets;
        std::set<unsigned int> nullableNonterminals;
        Base::grammar().computeSets(firstSets, followSets, nullableNonterminals);

        auto getReduceSet = [&](unsigned int state, unsigned int rule) {
            return followSets[rule];
        };

        if(computeParseTable(states, getReduceSet)) {
            mValid = true;
        }
    }
}