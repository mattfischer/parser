#include "DFA.hpp"

#include <iostream>
#include <algorithm>

DFA::DFA(const NFA &nfa)
{
    std::set<unsigned int> nfaStates;
    nfaStates.insert(nfa.startState());

    std::vector<StateSet> stateSets;
    mStartState = findOrAddState(stateSets, nfa, nfaStates);

    for(unsigned int i=0; i<stateSets.size(); i++) {
        const StateSet &stateSet = stateSets[i];
        State state;

        for(const auto &pair : stateSet.transitions) {
            state.transitions[pair.first] = pair.second;
        }

        if(stateSet.nfaStates.find(nfa.acceptState()) != stateSet.nfaStates.end()) {
            mAcceptStates.insert(i);
        }

        mStates.push_back(std::move(state));
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
    unsigned int idx = stateSets.size() - 1;
    stateSets[idx].nfaStates = epsilonClosure;
    
    for(const auto &pair : transitions) {
        stateSets[idx].transitions[pair.first] = findOrAddState(stateSets, nfa, pair.second);
    }

    return idx;
}

const std::vector<DFA::State> &DFA::states() const
{
    return mStates;
}

unsigned int DFA::startState() const
{
    return mStartState;
}

const std::set<unsigned int> &DFA::acceptStates() const
{
    return mAcceptStates;
}

void DFA::print() const
{
    std::cout << "Start state: " << mStartState << std::endl;
    std::cout << "Accept states: ";
    for(int state : mAcceptStates) {
        std::cout << state << " ";
    }
    std::cout << std::endl;
    for(int i=0; i<mStates.size(); i++) {
        std::cout << "State " << i << ":" << std::endl;
        for(const auto &pair : mStates[i].transitions) {
            std::cout << "  " << pair.first << " -> " << pair.second << std::endl;
        }
        std::cout << std::endl;
    }
}

void DFA::minimize()
{
    std::set<Symbol> alphabet;

    for(const auto &state : mStates) {
        for(const auto &transition : state.transitions) {
            alphabet.insert(transition.first);
        }
    }

    std::vector<std::set<unsigned int>> partition;

    partition.push_back(mAcceptStates);
    std::set<unsigned int> others;
    for(unsigned int i=0; i<mStates.size(); i++) {
        if(mAcceptStates.count(i) == 0) {
            others.insert(i);
        }
    }
    partition.push_back(std::move(others));

    std::vector<unsigned int> queue;
    queue.push_back(0);

    while(queue.size() > 0) {
        std::set<unsigned int> distinguisher = partition[queue.front()];
        queue.erase(queue.begin());

        for(Symbol c : alphabet) {
            std::set<unsigned int> inbound;
            for(unsigned int i=0; i<mStates.size(); i++) {
                const auto &state = mStates[i];
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
                    unsigned int o = partition.size() - 1;

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
        for(const auto &transition : mStates[s].transitions) {
            newState.transitions[transition.first] = stateMap[transition.second];
        }
        newStates.push_back(std::move(newState));
    }
    unsigned int newStartState = stateMap[mStartState];
    std::set<unsigned int> newAcceptStates;
    for(unsigned int s : mAcceptStates) {
        newAcceptStates.insert(stateMap[s]);
    }

    mStates = std::move(newStates);
    mStartState = newStartState;
    mAcceptStates = std::move(newAcceptStates);
}