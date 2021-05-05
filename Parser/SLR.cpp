#include "SLR.hpp"

namespace Parser
{
    SLR::SLR(const Grammar &grammar)
    : LR(grammar)
    {
        std::vector<State> states = computeStates();

        if(computeParseTable(states)) {
            mValid = true;
        }
    }

    bool SLR::computeParseTable(const std::vector<State> &states)
    {
        std::vector<std::set<unsigned int>> firstSets;
        std::vector<std::set<unsigned int>> followSets;
        std::set<unsigned int> nullableNonterminals;
        mGrammar.computeSets(firstSets, followSets, nullableNonterminals);

        mParseTable.resize(states.size(), mGrammar.terminals().size() + mGrammar.rules().size(), ParseTableEntry{ParseTableEntry::Type::Error, 0});
        for(unsigned int i=0; i<states.size(); i++) {
            for(const auto &item : states[i].items) {
                const Grammar::RHS &rhs = mGrammar.rules()[item.rule].rhs[item.rhs];
                if(item.pos == rhs.size()) {
                    for(unsigned int terminal : followSets[item.rule]) {
                        if(mParseTable.at(i, terminal).type != ParseTableEntry::Type::Error) {
                            mConflict.type = Conflict::Type::ReduceReduce;
                            mConflict.symbol = terminal;
                            mConflict.item1 = mParseTable.at(i, terminal).index;
                            mConflict.item2 = item.rule;
                            return false;
                        }

                        Reduction reduction{item.rule, item.rhs};
                        unsigned int index = (unsigned int)mReductions.size();
                        for(unsigned int j=0; j<mReductions.size(); j++) {
                            if(mReductions[j] == reduction) {
                                index = j;
                                break;
                            }
                        }
                        if(index == mReductions.size()) {
                            mReductions.push_back(reduction);
                        }
                        mParseTable.at(i, terminal) = ParseTableEntry{ParseTableEntry::Type::Reduce, index};
                    }

                    if(item.rule == mGrammar.startRule()) {
                        mAcceptStates.insert(i);
                    }
                }
            }

            for(const auto &transition : states[i].transitions) {
                if(mParseTable.at(i, transition.first).type != ParseTableEntry::Type::Error) {
                    mConflict.type = Conflict::Type::ShiftReduce;
                    mConflict.symbol = transition.first;
                    mConflict.item1 = mParseTable.at(i, transition.first).index;
                    return false;
                }
                mParseTable.at(i, transition.first) = ParseTableEntry{ParseTableEntry::Type::Shift, transition.second};
            }
        }

        return true;
    }
}