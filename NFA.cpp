#include "NFA.hpp"

#include <iostream>

NFA::NFA(const Parser::Node &node)
: mEncoding(constructInputSymbolRanges(node))
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
            addTransition(startState, mEncoding.codePoint(symbolNode.symbol), acceptState);
            break;
        }

        case Parser::Node::Type::CharacterClass:
        {
            const Parser::CharacterClassNode &characterClassNode = static_cast<const Parser::CharacterClassNode&>(node);
            for(const auto &range : characterClassNode.ranges) {
                std::vector<Symbol> symbols = mEncoding.codePoints(Encoding::InputSymbolRange(range.first, range.second));
                for(const auto &symbol : symbols) {
                    addTransition(startState, symbol, acceptState);
                }
            }
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
            std::cout << "  " << t.first << " -> " << t.second << std::endl;
        }
        std::cout << std::endl;
    }
}

static void visitNode(const Parser::Node &node, std::vector<Encoding::InputSymbolRange> &inputSymbolRanges)
{
    switch(node.type) {
        case Parser::Node::Type::Symbol:
        {
            const Parser::SymbolNode &symbolNode = static_cast<const Parser::SymbolNode&>(node);
            inputSymbolRanges.push_back(Encoding::InputSymbolRange(symbolNode.symbol, symbolNode.symbol));
            break;
        }

        case Parser::Node::Type::CharacterClass:
        {
            const Parser::CharacterClassNode &characterClassNode = static_cast<const Parser::CharacterClassNode&>(node);
            for(const auto &range : characterClassNode.ranges) {
                inputSymbolRanges.push_back(Encoding::InputSymbolRange(range.first, range.second));
            }
            break;
        }

        case Parser::Node::Type::OneOf:
        {
            const Parser::OneOfNode &oneOfNode = static_cast<const Parser::OneOfNode&>(node);
            for(const auto &child : oneOfNode.nodes) {
                visitNode(*child, inputSymbolRanges);
            }
            break;
        }

        case Parser::Node::Type::ZeroOrMore:
        {
            const Parser::ZeroOrMoreNode &zeroOrMoreNode = static_cast<const Parser::ZeroOrMoreNode&>(node);
            visitNode(*zeroOrMoreNode.node, inputSymbolRanges);
            break;
        }

        case Parser::Node::Type::OneOrMore:
        {
            const Parser::OneOrMoreNode &oneOrMoreNode = static_cast<const Parser::OneOrMoreNode&>(node);
            visitNode(*oneOrMoreNode.node, inputSymbolRanges);
            break;
        }

        case Parser::Node::Type::Sequence:
        {
            const Parser::SequenceNode &sequenceNode = static_cast<const Parser::SequenceNode&>(node);
            for(const auto &child : sequenceNode.nodes) {
                visitNode(*child, inputSymbolRanges);
            }
            break;
        }
    }
}

std::vector<Encoding::InputSymbolRange> NFA::constructInputSymbolRanges(const Parser::Node &node) const
{
    std::vector<Encoding::InputSymbolRange> inputSymbolRanges;

    visitNode(node, inputSymbolRanges);

    return inputSymbolRanges;
}