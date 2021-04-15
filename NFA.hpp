#ifndef NFA_HPP
#define NFA_HPP

#include <vector>
#include <utility>

#include "Parser.hpp"
#include "Encoding.hpp"

class NFA {
public:
    typedef Encoding::CodePoint Symbol;
    struct State {
        typedef std::pair<Symbol, int> Transition;
        std::vector<Transition> transitions;
        std::vector<int> epsilonTransitions;
    };

    NFA(const Parser::Node &node);

    int startState() const;
    int acceptState() const;
    const std::vector<State> &states() const;

    void print() const;

private:
    int addState();
    void addTransition(int fromState, Symbol symbol, int toState);
    void addEpsilonTransition(int fromState, int toState);
    void populate(const Parser::Node &node, int startState, int acceptState);
    std::vector<Encoding::InputSymbolRange> constructInputSymbolRanges(const Parser::Node &node) const;

    std::vector<State> mStates;
    int mStartState;
    int mAcceptState;
    Encoding mEncoding;
};

#endif