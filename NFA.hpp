#ifndef NFA_HPP
#define NFA_HPP

#include <vector>
#include <tuple>

#include "Parser.hpp"

class NFA {
public:
    typedef int Symbol;
    struct State {
        typedef std::tuple<Symbol, int> Transition;
        std::vector<Transition> transitions;
        std::vector<int> epsilonTransitions;
    };

    NFA(const Parser::Node &node);

    int startState() const;
    int acceptState() const;
    const std::vector<State> &states() const;

private:
    int addState();
    void addTransition(int fromState, Symbol symbol, int toState);
    void addEpsilonTransition(int fromState, int toState);
    void populate(const Parser::Node &node, int startState, int acceptState);

    std::vector<State> mStates;
    int mStartState;
    int mAcceptState;
};

#endif