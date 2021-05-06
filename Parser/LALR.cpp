#include "Parser/LALR.hpp"

#include <iostream>
#include <sstream>

namespace Parser
{
    LALR::LALR(const Grammar &grammar)
    : LR(grammar)
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

        std::vector<std::string> terminals = grammar.terminals();
        Grammar newGrammar(std::move(terminals), std::move(newRules), grammar.startRule());

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
}