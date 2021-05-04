#include "SLR.hpp"

#include <iostream>

namespace Parser
{
    SLR::SLR(const Grammar &grammar)
    : mGrammar(grammar)
    {
        mValid = false;
        std::vector<State> states = computeStates();
        printStates(states);

        if(computeParseTable(states)) {
            mValid = true;
        }
    }

    bool SLR::valid() const
    {
        return mValid;
    }

    const Grammar &SLR::grammar() const
    {
        return mGrammar;
    }

    const SLR::Conflict &SLR::conflict() const
    {
        return mConflict;
    }

    unsigned int SLR::symbolIndex(const Grammar::Symbol &symbol) const
    {
        switch(symbol.type) {
            case Grammar::Symbol::Type::Terminal:
                return terminalIndex(symbol.index);
            case Grammar::Symbol::Type::Nonterminal:
                return ruleIndex(symbol.index);
            default:
                return UINT_MAX;
        }
    }

    unsigned int SLR::terminalIndex(unsigned int terminal) const
    {
        return terminal;
    }

    unsigned int SLR::ruleIndex(unsigned int rule) const
    {
        return (unsigned int)mGrammar.terminals().size() + rule;
    }

    void SLR::computeClosure(std::set<Item> &items) const
    {
        std::vector<Item> queue;
        queue.insert(queue.begin(), items.begin(), items.end());

        auto addItem = [&](const Item &item) {
            if(items.count(item) == 0) {
                items.insert(item);
                queue.push_back(item);
            }
        };

        while(queue.size() > 0) {
            Item item = queue.front();
            queue.erase(queue.begin());

            const Grammar::RHS &rhs = mGrammar.rules()[item.rule].rhs[item.rhs];
            if(item.pos < rhs.size()) {
                switch(rhs[item.pos].type) {
                    case Grammar::Symbol::Type::Nonterminal:
                    {
                        unsigned int newRuleIndex = rhs[item.pos].index;
                        const Grammar::Rule &newRule = mGrammar.rules()[newRuleIndex];

                        for(unsigned int i=0; i<newRule.rhs.size(); i++) {
                            addItem(Item{newRuleIndex, i, 0});
                        }
                        break;
                    }

                    case Grammar::Symbol::Type::Epsilon:
                    {
                        addItem(Item{item.rule, item.rhs, item.pos + 1});
                        break;
                    }
                }
            }
        }
    }

    std::vector<SLR::State> SLR::computeStates() const
    {
        std::vector<State> states;

        State start;
        for(unsigned int i=0; i<mGrammar.rules()[mGrammar.startRule()].rhs.size(); i++) {
            start.items.insert(Item{mGrammar.startRule(), i, 0});
        }
        computeClosure(start.items);
        states.push_back(std::move(start));

        std::vector<unsigned int> queue;
        queue.push_back(0);

        while(queue.size() > 0) {
            unsigned int index = queue.front();
            queue.erase(queue.begin());

            for(unsigned int i=0; i<mGrammar.terminals().size() + mGrammar.rules().size(); i++) {
                State &state = states[index];
                std::set<Item> newItems;
            
                for(const auto &item : state.items) {
                    const Grammar::RHS &rhs = mGrammar.rules()[item.rule].rhs[item.rhs];
                    if(item.pos < rhs.size() && symbolIndex(rhs[item.pos]) == i) {
                        newItems.insert(Item{item.rule, item.rhs, item.pos + 1});
                    }
                }

                if(newItems.size() > 0) {
                    computeClosure(newItems);

                    bool found = false;
                    for(unsigned int j=0; j<states.size(); j++) {
                        if(states[j].items == newItems) {
                            state.transitions[i] = j;
                            found = true;
                            break;
                        }
                    }

                    if(!found) {
                        state.transitions[i] = (unsigned int)states.size();
                        queue.push_back((unsigned int)states.size());
                        states.push_back(State{std::move(newItems)});
                    }
                }
            }
        }

        return states;
    }

    bool SLR::computeParseTable(const std::vector<State> &states)
    {
        std::vector<std::set<unsigned int>> firstSets;
        std::vector<std::set<unsigned int>> followSets;
        std::set<unsigned int> nullableNonterminals;
        mGrammar.computeSets(firstSets, followSets, nullableNonterminals);

        for(unsigned int i=0; i<mGrammar.terminals().size(); i++) {
            followSets[mGrammar.startRule()].insert(i);
        }

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

    void SLR::printStates(const std::vector<State> &states) const
    {
        std::vector<std::set<unsigned int>> firstSets;
        std::vector<std::set<unsigned int>> followSets;
        std::set<unsigned int> nullableNonterminals;
        mGrammar.computeSets(firstSets, followSets, nullableNonterminals);

        for(unsigned int i=0; i<states.size(); i++) {
            std::cout << "State " << i << ":" << std::endl;
            for(const auto &item : states[i].items) {
                std::cout << "  <" << mGrammar.rules()[item.rule].lhs << ">: ";
                const Grammar::RHS &rhs = mGrammar.rules()[item.rule].rhs[item.rhs];
                for(unsigned int j=0; j<=rhs.size(); j++) {
                    if(j == item.pos) {
                        std::cout << ". ";
                    }
                    if(j == rhs.size()) {
                        break;
                    }

                    switch(rhs[j].type){
                        case Grammar::Symbol::Type::Terminal: 
                            std::cout << mGrammar.terminals()[rhs[j].index];
                            break;
                        
                        case Grammar::Symbol::Type::Nonterminal:
                            std::cout << "<" << mGrammar.rules()[rhs[j].index].lhs << ">";
                            break;
                        
                        case Grammar::Symbol::Type::Epsilon:
                            std::cout << "0";
                            break;
                    }
                    std::cout << " ";
                }
                if(item.pos == rhs.size()) {
                    std::cout << " [ ";
                    for(unsigned int s : followSets[item.rule]) {
                        std::cout << mGrammar.terminals()[s] << " ";
                    }
                    std::cout << "]";
                }
                std::cout << std::endl;
            }
            std::cout << std::endl;
            for(const auto &transition : states[i].transitions) {
                std::cout << "  ";
                if(transition.first < mGrammar.terminals().size()) {
                    std::cout << mGrammar.terminals()[transition.first];
                } else {
                    std::cout << "<" << mGrammar.rules()[transition.first - mGrammar.terminals().size()].lhs << ">";
                }
                std::cout << " -> " << transition.second << std::endl;
            }
            std::cout << std::endl;
        }
    }

}