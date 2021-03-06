#include "Parser/Impl/SLR.hpp"

namespace Parser
{
    namespace Impl
    {
        SLR::SLR(const Grammar &grammar)
        : LRSingle(grammar)
        {
            std::vector<State> states = computeStates();

            std::vector<std::set<unsigned int>> firstSets;
            std::vector<std::set<unsigned int>> followSets;
            std::set<unsigned int> nullableNonterminals;
            mGrammar.computeSets(firstSets, followSets, nullableNonterminals);

            auto getReduceSet = [&](unsigned int state, unsigned int rule) {
                return followSets[rule];
            };

            if(computeParseTable(states, getReduceSet)) {
                mValid = true;
            }
        }
    }
}