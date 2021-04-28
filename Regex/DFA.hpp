#ifndef REGEX_DFA_HPP
#define REGEX_DFA_HPP

#include "NFA.hpp"
#include "Encoding.hpp"

#include <map>
#include <set>

namespace Regex {
    class DFA {
    public:
        typedef int Symbol;

        DFA(const NFA &nfa, const Encoding &encoding);

        unsigned int startState() const;
        unsigned int rejectState() const;

        unsigned int transition(unsigned int state, Encoding::CodePoint codePoint) const;
        bool accept(unsigned int state, unsigned int &index) const;

        void print() const;

    private:  
        struct State {
            std::map<Symbol, unsigned int> transitions;
        };

        struct StateSet {
            std::set<unsigned int> nfaStates;
            std::map<DFA::Symbol, unsigned int> transitions;
        };

        unsigned int findOrAddState(std::vector<StateSet> &stateSets, const NFA &nfa, const std::set<unsigned int> &nfaStates);
        void minimize(std::vector<State> &states, unsigned int &startState, std::vector<unsigned int> &acceptStates);

        unsigned int mNumCodePoints;
        unsigned int mNumStates;
        unsigned int mStartState;
        unsigned int mRejectState;
        std::vector<unsigned int> mTransitions;
        std::vector<unsigned int> mAcceptStates;
    };
}

#endif