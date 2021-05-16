#include "Parser/GLR.hpp"

namespace Parser
{
    GLR::GLR(const Grammar &grammar)
    : LRMulti(grammar)
    {
        std::vector<State> states = computeStates();

        std::vector<std::set<unsigned int>> firstSets;
        std::vector<std::set<unsigned int>> followSets;
        std::set<unsigned int> nullableNonterminals;
        mGrammar.computeSets(firstSets, followSets, nullableNonterminals);

        auto getReduceSet = [&](unsigned int state, unsigned int rule) {
            return followSets[rule];
        };

        computeParseTable(states, getReduceSet);
    }
}