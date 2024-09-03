#include "Parser/Impl/LRTable/Single.hpp"

namespace Parser::Impl::LRTable {
    Single::Single(const Grammar &grammar)
    : Base(grammar)
    {
    }

    bool Single::valid() const
    {
        return mValid;
    }

    const Single::Conflict &Single::conflict() const
    {
        return mConflict;
    }

    bool Single::isAccept(unsigned int state) const
    {
        return mAcceptStates.contains(state);
    }

    unsigned int Single::nextStateForRule(unsigned int state, unsigned int rule) const
    {
        return mParseTable.at(state, ruleIndex(rule)).index;
    }

    void Single::computeParseTable(const std::vector<State> &states)
    {
        mParseTable.resize(states.size(), grammar().terminals().size() + grammar().rules().size(), ParseTableEntry{ParseTableEntry::Type::Error, 0});
        for(unsigned int i=0; i<states.size(); i++) {
            for(const auto &item : states[i].items) {
                const Grammar::RHS &rhs = grammar().rules()[item.rule].rhs[item.rhs];
                if(item.pos == rhs.size()) {
                    for(unsigned int terminal : getReduceLookahead(i, item.rule)) {
                        if(mParseTable.at(i, terminal).type != ParseTableEntry::Type::Error) {
                            mConflict.type = Conflict::Type::ReduceReduce;
                            mConflict.symbol = terminal;
                            mConflict.item1 = mParseTable.at(i, terminal).index;
                            mConflict.item2 = item.rule;
                            mValid = false;
                            return;
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

                    if(item.rule == grammar().startRule()) {
                        mAcceptStates.insert(i);
                    }
                }
            }

            for(const auto &transition : states[i].transitions) {
                if(mParseTable.at(i, transition.first).type != ParseTableEntry::Type::Error) {
                    mConflict.type = Conflict::Type::ShiftReduce;
                    mConflict.symbol = transition.first;
                    mConflict.item1 = mParseTable.at(i, transition.first).index;
                    mValid = false;
                    return;
                }
                mParseTable.at(i, transition.first) = ParseTableEntry{ParseTableEntry::Type::Shift, transition.second};
            }
        }

        mValid = true;
    }
}