#include "NFA.hpp"

#include <iostream>

namespace Regex {

    NFA::NFA(const Parser::Node &node, const Encoding &encoding)
    {
        mStartState = addState();
        mAcceptState = addState();
        populate(node, encoding, mStartState, mAcceptState);
    }

    unsigned int NFA::startState() const
    {
        return mStartState;
    }

    unsigned int NFA::acceptState() const
    {
        return mAcceptState;
    }

    const std::vector<NFA::State> &NFA::states() const
    {
        return mStates;
    }

    unsigned int NFA::addState()
    {
        mStates.push_back(State());
        return (unsigned int)(mStates.size() - 1);
    }

    void NFA::addTransition(unsigned int fromState, Symbol symbol, unsigned int toState)
    {
        mStates[fromState].transitions.push_back(State::Transition(symbol, toState));
    }

    void NFA::addEpsilonTransition(unsigned int fromState, unsigned int toState)
    {
        mStates[fromState].epsilonTransitions.push_back(toState);
    }

    void NFA::populate(const Parser::Node &node, const Encoding &encoding, unsigned int startState, unsigned int acceptState)
    {
        switch(node.type) {
            case Parser::Node::Type::Symbol:
            {
                const Parser::SymbolNode &symbolNode = static_cast<const Parser::SymbolNode&>(node);
                addTransition(startState, encoding.codePoint(symbolNode.symbol), acceptState);
                break;
            }

            case Parser::Node::Type::CharacterClass:
            {
                const Parser::CharacterClassNode &characterClassNode = static_cast<const Parser::CharacterClassNode&>(node);
                for(const auto &range : characterClassNode.ranges) {
                    std::vector<Symbol> symbols = encoding.codePointRanges(Encoding::InputSymbolRange(range.first, range.second));
                    for(const auto &symbol : symbols) {
                        addTransition(startState, symbol, acceptState);
                    }
                }
                break;
            }

            case Parser::Node::Type::Sequence:
            {
                const Parser::SequenceNode &sequenceNode = static_cast<const Parser::SequenceNode&>(node);
                unsigned int current = startState;
                for(unsigned int i=0; i<sequenceNode.nodes.size(); i++) {
                    unsigned int next = addState();                
                    populate(*sequenceNode.nodes[i], encoding, current, next);
                    current = next;
                }
                addEpsilonTransition(current, acceptState);
                break;
            }

            case Parser::Node::Type::ZeroOrOne:
            {
                const Parser::ZeroOrOneNode &zeroOrOneNode = static_cast<const Parser::ZeroOrOneNode&>(node);
                unsigned int first = addState();
                addEpsilonTransition(startState, first);

                unsigned int next = addState();
                addEpsilonTransition(next, acceptState);

                populate(*zeroOrOneNode.node, encoding, first, next);
                addEpsilonTransition(first, next);
                break;
            }

            case Parser::Node::Type::ZeroOrMore:
            {
                const Parser::ZeroOrMoreNode &zeroOrMoreNode = static_cast<const Parser::ZeroOrMoreNode&>(node);
                unsigned int first = addState();
                addEpsilonTransition(startState, first);

                unsigned int next = addState();
                addEpsilonTransition(next, acceptState);

                populate(*zeroOrMoreNode.node, encoding, first, next);
                addEpsilonTransition(first, next);
                addEpsilonTransition(next, first);
                break;
            }

            case Parser::Node::Type::OneOrMore:
            {
                const Parser::OneOrMoreNode &oneOrMoreNode = static_cast<const Parser::OneOrMoreNode&>(node);
                unsigned int first = addState();
                addEpsilonTransition(startState, first);

                unsigned int next = addState();
                addEpsilonTransition(next, acceptState);

                populate(*oneOrMoreNode.node, encoding, first, next);
                addEpsilonTransition(next, first);
                break;
            }

            case Parser::Node::Type::OneOf:
            {
                const Parser::OneOfNode &oneOfNode = static_cast<const Parser::OneOfNode&>(node);
                unsigned int newStart = addState();
                addEpsilonTransition(startState, newStart);

                unsigned int newAccept = addState();
                addEpsilonTransition(newAccept, acceptState);
                for(auto &n : oneOfNode.nodes) {
                    populate(*n, encoding, newStart, newAccept);
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
        for(unsigned int i=0; i<mStates.size(); i++) {
            const State &state = mStates[i];
            std::cout << "State " << i << ":" << std::endl;
            for(unsigned int s : state.epsilonTransitions) {
                std::cout << "  -> " << s << std::endl;
            }
            for(const auto &t : state.transitions) {
                std::cout << "  " << t.first << " -> " << t.second << std::endl;
            }
            std::cout << std::endl;
        }
    }
}