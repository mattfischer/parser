#ifndef REGEX_NFA_HPP
#define REGEX_NFA_HPP

#include <vector>
#include <utility>

#include "Parser.hpp"
#include "Encoding.hpp"

namespace Regex {

    class NFA {
    public:
        typedef Encoding::CodePoint Symbol;
        struct State {
            typedef std::pair<Symbol, unsigned int> Transition;
            std::vector<Transition> transitions;
            std::vector<unsigned int> epsilonTransitions;
        };

        NFA(const std::vector<std::unique_ptr<Parser::Node>> &nodes, const Encoding &encoding);

        unsigned int startState() const;
        const std::vector<unsigned int> &acceptStates() const;
        const std::vector<State> &states() const;

        void print() const;

    private:
        unsigned int addState();
        void addTransition(unsigned int fromState, Symbol symbol, unsigned int toState);
        void addEpsilonTransition(unsigned int fromState, unsigned int toState);
        void populate(const Parser::Node &node, const Encoding &encoding, unsigned int startState, unsigned int acceptState);
        
        std::vector<State> mStates;
        unsigned int mStartState;
        std::vector<unsigned int> mAcceptStates;
    };
}

#endif