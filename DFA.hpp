#ifndef DFA_HPP
#define DFA_HPP

#include "NFA.hpp"

#include <map>
#include <set>

class DFA {
public:
    typedef int Symbol;

    struct State {
        std::map<Symbol, int> transitions;
    };

    DFA(const NFA &nfa);

    void print() const;

private:  
    struct StateSet {
        std::set<int> nfaStates;
        std::map<DFA::Symbol, int> transitions;
    };

    int findOrAddState(std::vector<StateSet> &stateSets, const NFA &nfa, const std::set<int> &nfaStates);

    int mStartState;
    std::vector<int> mAcceptStates;
    std::vector<State> mStates;
};

#endif