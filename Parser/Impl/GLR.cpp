#include "Parser/Impl/GLR.hpp"

namespace Parser::Impl
{
    GLRBase::GLRBase(const Grammar &grammar)
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

    void GLRBase::runParse(Tokenizer::Stream &stream, ParseStacksBase &stacks) const
    {
        stacks.pushState(0, 0);

        while(true) {
            bool repeat = false;
            for(size_t i=0; i<stacks.size() || repeat; i++) {
                if(repeat) {
                    i--;
                    repeat = false;
                    if(i >= stacks.size()) {
                        break;
                    }
                }

                unsigned int state = stacks.backState(i);
                if(mAcceptStates.contains(state)) {
                    continue;
                }

                const ParseTableEntry &entry = mParseTable.at(state, stream.nextToken().value);
                switch(entry.type) {
                    case ParseTableEntry::Type::Shift:
                    {
                        shift(stream.nextToken(), entry.index, stacks, i);
                        break;
                    }

                    case ParseTableEntry::Type::Reduce:
                    {
                        const Reduction &reduction = mReductions[entry.index];
                        reduce(reduction.rule, reduction.rhs, stacks, i, false);
                        repeat = true;
                        break;
                    }

                    case ParseTableEntry::Type::Multi:
                    {
                        const auto &entries = mMultiEntries[entry.index];
                        for(size_t j=0; j<entries.size(); j++) {
                            const auto &entry = entries[j];
                            switch(entry.type) {
                                case ParseTableEntry::Type::Shift:
                                {
                                    shift(stream.nextToken(), entry.index, stacks, i);
                                    break;
                                }

                                case ParseTableEntry::Type::Reduce:
                                {
                                    const Reduction &reduction = mReductions[entry.index];
                                    bool preserveStack = true;
                                    if(j == entries.size() - 1) {
                                        preserveStack = false;
                                        repeat = true;
                                    }
                                    reduce(reduction.rule, reduction.rhs, stacks, i, preserveStack);
                                    break;
                                }

                                default:
                                    break;
                            }
                        }
                        break;
                    }

                    case ParseTableEntry::Type::Error:
                    {
                        stacks.eraseStack(i);
                        repeat = true;
                        break;
                    }
                }
            }

            if(stream.nextToken().value == stream.tokenizer().endValue()) {
                break;
            }
            stream.consumeToken();

            if(stacks.size() > 1) {
                std::map<unsigned int, size_t> stackMap;
                repeat = false;
                for(size_t i=0; i<stacks.size() || repeat; i++) {
                    if(repeat) {
                        i--;
                        repeat = false;
                        if(i >= stacks.size()) {
                            break;
                        }
                    }

                    unsigned int state = stacks.backState(i);
                    auto it = stackMap.find(state);
                    if(it == stackMap.end()) {
                        stackMap[state] = i;
                    } else {
                        stacks.joinStacks(i, it->second);
                        repeat = true;
                    }
                }
            }
        }

        for(size_t i=0; i<stacks.size(); i++) {
            reduce(grammar().startRule(), 0, stacks, i, false);
        }
    }
}