#include "Parser/Impl/LRTable.hpp"

#include <iostream>
#include <sstream>

namespace Parser::Impl::LRTable {
    Base::Base(const Grammar &grammar)
    : mGrammar(grammar)
    {
    }

    const Grammar &Base::grammar() const
    {
        return mGrammar;
    }

    bool Base::Item::operator<(const Item &other) const {
        if(rule < other.rule) return true;
        if(rule > other.rule) return false;
        if(rhs < other.rhs) return true;
        if(rhs > other.rhs) return false;
        if(pos < other.pos) return true;
        return false;
    }

    bool Base::Item::operator==(const Item &other) const {
        return rule == other.rule && rhs == other.rhs && pos == other.pos;
    }

    unsigned int Base::symbolIndex(const Grammar::Symbol &symbol) const
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

    unsigned int Base::terminalIndex(unsigned int terminal) const
    {
        return terminal;
    }

    unsigned int Base::ruleIndex(unsigned int rule) const
    {
        return (unsigned int)mGrammar.terminals().size() + rule;
    }

    void Base::computeClosure(std::set<Item> &items) const
    {
        std::vector<Item> queue;
        queue.insert(queue.begin(), items.begin(), items.end());

        auto addItem = [&](const Item &item) {
            if(!items.contains(item)) {
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

    std::vector<Base::State> Base::computeStates() const
    {
        std::vector<State> states;

        State start;
        for(unsigned int i=0; i<grammar().rules()[grammar().startRule()].rhs.size(); i++) {
            start.items.insert(Item{grammar().startRule(), i, 0});
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

    void Base::printStates(const std::vector<State> &states, GetReduceLookahead getReduceLookahead) const
    {
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
                    for(unsigned int s : getReduceLookahead(i, item.rule)) {
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

    Single::Single(const Grammar &grammar)
    : Base(grammar)
    {
    }

    bool Single::isAccept(unsigned int state) const
    {
        return mAcceptStates.contains(state);
    }

    unsigned int Single::nextStateForRule(unsigned int state, unsigned int rule) const
    {
        return mParseTable.at(state, ruleIndex(rule)).index;
    }

    bool Single::computeParseTable(const std::vector<State> &states, GetReduceLookahead getReduceLookahead)
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
                    return false;
                }
                mParseTable.at(i, transition.first) = ParseTableEntry{ParseTableEntry::Type::Shift, transition.second};
            }
        }

        return true;
    }

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

    LALR::LALR(const Grammar &grammar)
    : Single(grammar)
    {
        std::vector<State> states = computeStates();

        std::vector<std::pair<unsigned int, unsigned int>> newNonterminals;
        auto findNonterminal = [&](unsigned int state, unsigned int rule) {
            for(unsigned int i=0; i<newNonterminals.size(); i++) {
                if(newNonterminals[i] == std::make_pair(state, rule)) { return i;}
            }
            return UINT_MAX;
        };

        std::vector<Grammar::Rule> newRules;
        for(unsigned int i=0; i<states.size(); i++) {
            for(const auto &item : states[i].items) {
                if(item.pos == 0 && findNonterminal(i, item.rule) == UINT_MAX) {
                    newNonterminals.push_back(std::make_pair(i, item.rule));
                    std::stringstream ss;
                    ss << grammar.rules()[item.rule].lhs << "@" << i;
                    newRules.push_back(Grammar::Rule{ss.str()});        
                }
            }
        }

        std::map<std::pair<unsigned int, unsigned int>, std::set<unsigned int>> reductionStarts;
        for(unsigned int i=0; i<states.size(); i++) {
            const State &state = states[i];

            for(const auto &item : state.items) {
                if(item.pos == 0) {
                    const Grammar::RHS &rhs = grammar.rules()[item.rule].rhs[item.rhs];
                    
                    Grammar::RHS newRhs;
                    unsigned int stateNum = i;
                    for(unsigned int j=0; j<rhs.size(); j++) {
                        switch(rhs[j].type) {
                            case Grammar::Symbol::Type::Nonterminal:
                            {
                                unsigned int s = findNonterminal(stateNum, rhs[j].index);
                                newRhs.push_back(Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, s});
                                auto it = states[stateNum].transitions.find(symbolIndex(rhs[j]));
                                stateNum = it->second;
                                break;
                            }
                            case Grammar::Symbol::Type::Terminal:
                            {
                                newRhs.push_back(rhs[j]);
                                auto it = states[stateNum].transitions.find(symbolIndex(rhs[j]));
                                stateNum = it->second;
                                break;
                            }
                            case Grammar::Symbol::Type::Epsilon:
                            {
                                newRhs.push_back(rhs[j]);
                                break;
                            }
                        }
                    }

                    unsigned int r = findNonterminal(i, item.rule);
                    newRules[r].rhs.push_back(std::move(newRhs));
    
                    reductionStarts[std::make_pair(stateNum, item.rule)].insert(i);
                }    
            }
        }

        Grammar newGrammar(grammar.terminals(), std::move(newRules), grammar.startRule());

        std::vector<std::set<unsigned int>> firstSets;
        std::vector<std::set<unsigned int>> followSets;
        std::set<unsigned int> nullableTerminals;
        newGrammar.computeSets(firstSets, followSets, nullableTerminals);

        std::map<std::pair<unsigned int, unsigned int>, std::set<unsigned int>> followPerStateSets;
        for(const auto &it : reductionStarts) {
            unsigned int reduceState = it.first.first;
            unsigned int rule = it.first.second;
            for(unsigned int startState : it.second) {
                unsigned int r = findNonterminal(startState, rule);
                followPerStateSets[std::make_pair(reduceState, rule)].insert(followSets[r].begin(), followSets[r].end());
            }
        }

        auto getReduceLookahead = [&](unsigned int state, unsigned int rule) {
            return followPerStateSets[std::make_pair(state, rule)];
        };

        if(computeParseTable(states, getReduceLookahead)) {
            mValid = true;
        }
    }

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