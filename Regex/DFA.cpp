#include "DFA.hpp"

#include <iostream>
#include <algorithm>
#include <climits>

namespace Regex {
    DFA::DFA(const NFA &nfa, const Encoding &encoding)
    {
        std::set<unsigned int> nfaStates;
        nfaStates.insert(nfa.startState());

        std::vector<StateSet> stateSets;
        std::vector<State> states;
        unsigned int startState = findOrAddState(stateSets, nfa, nfaStates);

        std::vector<std::set<unsigned int>> acceptSets;
        acceptSets.resize(nfa.acceptStates().size());
    
        for(unsigned int i=0; i<stateSets.size(); i++) {
            const StateSet &stateSet = stateSets[i];
            State state;

            for(const auto &pair : stateSet.transitions) {
                state.transitions[pair.first] = pair.second;
            }

            for(unsigned int j=0; j<nfa.acceptStates().size(); j++) { 
                if(stateSet.nfaStates.find(nfa.acceptStates()[j]) != stateSet.nfaStates.end()) {
                    acceptSets[j].insert(i);
                }
            }

            states.push_back(std::move(state));
        }

        std::vector<unsigned int> acceptStates;
        acceptStates.resize(states.size());
        for(unsigned int i=0; i<acceptStates.size(); i++) {
            const State &state = states[i];
            acceptStates[i] = UINT_MAX;
            for(unsigned int j=0; j<acceptSets.size(); j++) {
                if(acceptSets[j].count(i) > 0) {
                    acceptStates[i] = j;
                    break;
                }
            }
        }

        minimize(states, startState, acceptStates);

        mStartState = startState;
        states.push_back(State());
        mNumStates = (unsigned int)states.size();
        mNumCodePoints = encoding.numCodePoints();
        mRejectState = mNumStates - 1;
        mTransitions.resize(mNumStates, mNumCodePoints);
        mAcceptStates = std::move(acceptStates);

        for(unsigned int i=0; i<mNumStates; i++) {
            const State &state = states[i];

            for(unsigned int j=0; j<mNumCodePoints; j++) {
                auto it = state.transitions.find(j);
                if(it == state.transitions.end()) {
                    mTransitions.at(i, j) = mRejectState;
                } else {
                    mTransitions.at(i, j) = it->second;
                }
            }
        }
    }

    unsigned int DFA::startState() const
    {
        return mStartState;
    }

    unsigned int DFA::rejectState() const
    {
        return mRejectState;
    }

    unsigned int DFA::transition(unsigned int state, Encoding::CodePoint codePoint) const
    {
        return mTransitions.at(state, codePoint);
    }

    bool DFA::accept(unsigned int state, unsigned int &index) const
    {
        if(mAcceptStates[state] == UINT_MAX) {
            return false;
        } else {
            index = mAcceptStates[state];
            return true;
        }
    }

    void DFA::print() const
    {
        std::cout << "Start state: " << mStartState << std::endl;
        std::cout << "Accept states: ";
        for(unsigned int i = 0; i < mAcceptStates.size(); i++) {
            if(mAcceptStates[i]) {
                std::cout << i << " ";
            }
        }
        std::cout << std::endl;

        for(unsigned int i=0; i<mNumStates; i++) {
            std::cout << "State " << i << ":" << std::endl;
            for(unsigned int j = 0; j < mNumCodePoints; j++) {
                unsigned int nextState = transition(i, j);
                if(nextState != mRejectState) {
                    std::cout << "  " << i << " -> " << j << std::endl;
                }
            }
            std::cout << std::endl;
        }
    }

    unsigned int DFA::findOrAddState(std::vector<StateSet> &stateSets, const NFA &nfa, const std::set<unsigned int> &nfaStates)
    {
        std::set<unsigned int> epsilonClosure;
        std::vector<unsigned int> queue;
        queue.insert(queue.end(), nfaStates.begin(), nfaStates.end());
        while(queue.size() > 0) {
            unsigned int state = queue.front();
            queue.erase(queue.begin());
            if(epsilonClosure.find(state) == epsilonClosure.end()) {
                epsilonClosure.insert(state);
                const auto &transitions = nfa.states()[state].epsilonTransitions;
                queue.insert(queue.end(), transitions.cbegin(), transitions.cend());
            }
        }

        for(unsigned int i=0; i<stateSets.size(); i++) {
            if(stateSets[i].nfaStates == epsilonClosure) {
                return i;
            }
        }

        std::map<Symbol, std::set<unsigned int>> transitions;
        for(unsigned int state : epsilonClosure) {
            for(const NFA::State::Transition &transition : nfa.states()[state].transitions) {
                transitions[transition.first].insert(transition.second);
            }
        }
        
        stateSets.push_back(StateSet());
        unsigned int idx = (unsigned int)(stateSets.size() - 1);
        stateSets[idx].nfaStates = epsilonClosure;
        
        for(const auto &pair : transitions) {
            stateSets[idx].transitions[pair.first] = findOrAddState(stateSets, nfa, pair.second);
        }

        return idx;
    }

    void DFA::minimize(std::vector<State> &states, unsigned int &startState, std::vector<unsigned int> &acceptStates)
    {
        std::set<Symbol> alphabet;

        for(const auto &state : states) {
            for(const auto &transition : state.transitions) {
                alphabet.insert(transition.first);
            }
        }

        std::map<unsigned int, std::set<unsigned int>> acceptSets;

        for(unsigned int i=0; i<acceptStates.size(); i++) {
            acceptSets[acceptStates[i]].insert(i);
        }

        std::vector<std::set<unsigned int>> partition;
        for(auto &pair : acceptSets) {
            partition.push_back(std::move(pair.second));
        }

        std::vector<unsigned int> queue;
        for(unsigned int i = 0; i<acceptStates.size(); i++) {
            queue.push_back(i);
        }

        while(queue.size() > 0) {
            std::set<unsigned int> distinguisher = partition[queue.front()];
            queue.erase(queue.begin());

            for(Symbol c : alphabet) {
                std::set<unsigned int> inbound;
                for(unsigned int i=0; i<states.size(); i++) {
                    const auto &state = states[i];
                    const auto it = state.transitions.find(c);
                    if(it != state.transitions.end() && distinguisher.count(it->second)) {
                        inbound.insert(i);
                    }
                }

                if(inbound.size() == 0) {
                    continue;
                }

                for(unsigned int i=0; i<partition.size(); i++) {
                    std::set<unsigned int> in;
                    std::set<unsigned int> out;

                    for(int s : partition[i]) {
                        if(inbound.count(s)) {
                            in.insert(s);
                        } else {
                            out.insert(s);
                        }
                    }

                    if(in.size() > 0 && out.size() > 0) {
                        partition[i] = in;
                        partition.push_back(out);
                        unsigned int o = (unsigned int)(partition.size() - 1);

                        auto it = std::find(queue.begin(), queue.end(), i);
                        if(it == queue.end()) {
                            if(in.size() > out.size()) {
                                queue.push_back(o);
                            } else {
                                queue.push_back(i);
                            }
                        } else {
                            queue.push_back(o);
                        }
                    }
                }
            }       
        }

        std::map<unsigned int, unsigned int> stateMap;
        for(unsigned int i=0; i<partition.size(); i++) {
            for(unsigned int j : partition[i]) {
                stateMap[j] = i;
            }
        }

        std::vector<State> newStates;
        for(unsigned int i=0; i<partition.size(); i++) {
            State newState;
            int s = *partition[i].begin();
            for(const auto &transition : states[s].transitions) {
                newState.transitions[transition.first] = stateMap[transition.second];
            }
            newStates.push_back(std::move(newState));
        }
        unsigned int newStartState = stateMap[startState];
        std::vector<unsigned int> newAcceptStates;
        newAcceptStates.resize(newStates.size());
        for(unsigned int i=0; i<states.size(); i++) {
            newAcceptStates[stateMap[i]] = acceptStates[i];
        }

        states = std::move(newStates);
        startState = newStartState;
        acceptStates = std::move(newAcceptStates);
    }
}