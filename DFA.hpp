#ifndef DFA_HPP
#define DFA_HPP

#include "NFA.hpp"

#include <map>
#include <set>

class DFA {
public:
    typedef int Symbol;

    struct State {
        std::map<Symbol, unsigned int> transitions;
    };

    DFA(const NFA &nfa);

    const std::vector<State> &states() const;
    unsigned int startState() const;
    const std::set<unsigned int> &acceptStates() const;

    void print() const;

    void minimize();

private:  
    struct StateSet {
        std::set<unsigned int> nfaStates;
        std::map<DFA::Symbol, unsigned int> transitions;
    };

    unsigned int findOrAddState(std::vector<StateSet> &stateSets, const NFA &nfa, const std::set<unsigned int> &nfaStates);

    unsigned int mStartState;
    std::set<unsigned int> mAcceptStates;
    std::vector<State> mStates;
};

#endif