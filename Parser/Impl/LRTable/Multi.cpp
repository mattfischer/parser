#include "Parser/Impl/LRTable/Multi.hpp"

namespace Parser::Impl::LRTable {
    Multi::Multi(const Grammar &grammar)
    : Base(grammar)
    {
        std::vector<State> states = computeStates();

        std::vector<std::set<unsigned int>> firstSets;
        std::vector<std::set<unsigned int>> followSets;
        std::set<unsigned int> nullableNonterminals;
        Base::grammar().computeSets(firstSets, followSets, nullableNonterminals);

        auto getReduceSet = [&](unsigned int state, unsigned int rule) {
            return followSets[rule];
        };

        computeParseTable(states, getReduceSet);
    }

    bool Multi::isAccept(unsigned int state) const
    {
        return mAcceptStates.contains(state);
    }

    unsigned int Multi::nextStateForRule(unsigned int state, unsigned int rule) const
    {
        return mParseTable.at(state, ruleIndex(rule)).index;
    }

    void Multi::addParseTableEntry(unsigned int state, unsigned int symbol, const ParseTableEntry &entry)
    {
        switch(mParseTable.at(state, symbol).type) {
            case ParseTableEntry::Type::Error:
                mParseTable.at(state, symbol) = entry;
                break;
            case ParseTableEntry::Type::Multi:
                mMultiEntries[mParseTable.at(state, symbol).index].push_back(entry);
                break;
            case ParseTableEntry::Type::Shift:
            case ParseTableEntry::Type::Reduce:
                mMultiEntries.push_back(std::vector<ParseTableEntry>{mParseTable.at(state, symbol), entry});
                mParseTable.at(state, symbol) = ParseTableEntry{ParseTableEntry::Type::Multi, (unsigned int)(mMultiEntries.size() - 1)};
                break;
        }
    }

    void Multi::computeParseTable(const std::vector<State> &states, GetReduceLookahead getReduceLookahead)
    {
        mParseTable.resize(states.size(), grammar().terminals().size() + grammar().rules().size(), ParseTableEntry{ParseTableEntry::Type::Error, 0});
        for(unsigned int i=0; i<states.size(); i++) {
            for(const auto &item : states[i].items) {
                const Grammar::RHS &rhs = grammar().rules()[item.rule].rhs[item.rhs];
                if(item.pos == rhs.size()) {
                    for(unsigned int terminal : getReduceLookahead(i, item.rule)) {
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
                        addParseTableEntry(i, terminal, ParseTableEntry{ParseTableEntry::Type::Reduce, index});
                    }

                    if(item.rule == grammar().startRule()) {
                        mAcceptStates.insert(i);
                    }
                }
            }

            for(const auto &transition : states[i].transitions) {
                addParseTableEntry(i, transition.first, ParseTableEntry{ParseTableEntry::Type::Shift, transition.second});
            }
        }
    }
}