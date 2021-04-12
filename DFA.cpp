#include "DFA.hpp"

#include <iostream>

DFA::DFA(const NFA &nfa)
{
    std::set<int> nfaStates;
    nfaStates.insert(nfa.startState());

    std::vector<StateSet> stateSets;
    mStartState = findOrAddState(stateSets, nfa, nfaStates);

    for(int i=0; i<stateSets.size(); i++) {
        const StateSet &stateSet = stateSets[i];
        State state;

        for(const auto &pair : stateSet.transitions) {
            state.transitions[pair.first] = pair.second;
        }

        if(stateSet.nfaStates.find(nfa.acceptState()) != stateSet.nfaStates.end()) {
            mAcceptStates.push_back(i);
        }

        mStates.push_back(std::move(state));
    }
}

int DFA::findOrAddState(std::vector<StateSet> &stateSets, const NFA &nfa, const std::set<int> &nfaStates)
{
    std::set<int> epsilonClosure;
    std::vector<int> queue;
    queue.insert(queue.end(), nfaStates.begin(), nfaStates.end());
    while(queue.size() > 0) {
        int state = queue.front();
        queue.erase(queue.begin());
        if(epsilonClosure.find(state) == epsilonClosure.end()) {
            epsilonClosure.insert(state);
            const auto &transitions = nfa.states()[state].epsilonTransitions;
            queue.insert(queue.end(), transitions.cbegin(), transitions.cend());
        }
    }

    for(int i=0; i<stateSets.size(); i++) {
        if(stateSets[i].nfaStates == epsilonClosure) {
            return i;
        }
    }

    std::map<Symbol, std::set<int>> transitions;
    for(int state : epsilonClosure) {
        for(const NFA::State::Transition &transition : nfa.states()[state].transitions) {
            transitions[std::get<0>(transition)].insert(std::get<1>(transition));
        }
    }
    
    stateSets.push_back(StateSet());
    int idx = int(stateSets.size() - 1);
    stateSets[idx].nfaStates = epsilonClosure;
    
    for(const auto &pair : transitions) {
        stateSets[idx].transitions[pair.first] = findOrAddState(stateSets, nfa, pair.second);
    }

    return idx;
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