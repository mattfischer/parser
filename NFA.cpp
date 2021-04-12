#include "NFA.hpp"

#include <iostream>

NFA::NFA(const Parser::Node &node)
{
    mStartState = addState();
    mAcceptState = addState();
    populate(node, mStartState, mAcceptState);
}

int NFA::startState() const
{
    return mStartState;
}

int NFA::acceptState() const
{
    return mAcceptState;
}

const std::vector<NFA::State> &NFA::states() const
{
    return mStates;
}


int NFA::addState()
{
    mStates.push_back(State());
    return int(mStates.size() - 1);
}

void NFA::addTransition(int fromState, Symbol symbol, int toState)
{
    mStates[fromState].transitions.push_back(State::Transition(symbol, toState));
}

void NFA::addEpsilonTransition(int fromState, int toState)
{
    mStates[fromState].epsilonTransitions.push_back(toState);
}

void NFA::populate(const Parser::Node &node, int startState, int acceptState)
{
    switch(node.type) {
        case Parser::Node::Type::Symbol:
        {
            const Parser::SymbolNode &symbolNode = static_cast<const Parser::SymbolNode&>(node);
            addTransition(startState, symbolNode.symbol, acceptState);
            break;
        }

        case Parser::Node::Type::Sequence:
        {
            const Parser::SequenceNode &sequenceNode = static_cast<const Parser::SequenceNode&>(node);
            int current = startState;
            for(int i=0; i<sequenceNode.nodes.size(); i++) {
                int next = addState();                
                populate(*sequenceNode.nodes[i], current, next);
                current = next;
            }
            addEpsilonTransition(current, acceptState);
            break;
        }

        case Parser::Node::Type::ZeroOrMore:
        {
            const Parser::ZeroOrMoreNode &zeroOrMoreNode = static_cast<const Parser::ZeroOrMoreNode&>(node);
            int first = addState();
            addEpsilonTransition(startState, first);

            int next = addState();
            addEpsilonTransition(next, acceptState);

            populate(*zeroOrMoreNode.node, first, next);
            addEpsilonTransition(first, next);
            addEpsilonTransition(next, first);
            break;
        }

        case Parser::Node::Type::OneOrMore:
        {
            const Parser::OneOrMoreNode &oneOrMoreNode = static_cast<const Parser::OneOrMoreNode&>(node);
            int first = addState();
            addEpsilonTransition(startState, first);

            int next = addState();
            addEpsilonTransition(next, acceptState);

            populate(*oneOrMoreNode.node, first, next);
            addEpsilonTransition(next, first);
            break;
        }

        case Parser::Node::Type::OneOf:
        {
            const Parser::OneOfNode &oneOfNode = static_cast<const Parser::OneOfNode&>(node);
            int newStart = addState();
            addEpsilonTransition(startState, newStart);

            int newAccept = addState();
            addEpsilonTransition(newAccept, acceptState);
            for(auto &n : oneOfNode.nodes) {
                populate(*n, newStart, newAccept);
            }
            break;
        }
    }
}

void NFA::print() const
{
    std::cout << "Start: " << mStartState << std::endl;
    std::cout << "Accept: " << mAcceptState << std::endl;
    std::cout << std::endl;
    for(int i=0; i<mStates.size(); i++) {
        const State &state = mStates[i];
        std::cout << "State " << i << ":" << std::endl;
        for(int s : state.epsilonTransitions) {
            std::cout << "  -> " << s << std::endl;
        }
        for(const auto &t : state.transitions) {
            std::cout << "  " << std::get<0>(t) << " -> " << std::get<1>(t) << std::endl;
        }
        std::cout << std::endl;
    }
}