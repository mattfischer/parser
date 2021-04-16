#include "Matcher.hpp"

#include "NFA.hpp"

#include <sstream>

Matcher::Matcher(const std::string &pattern)
{
    std::unique_ptr<Parser::Node> node;
    
    try {
        node = Parser::parse(pattern);
    } catch(Parser::ParseException e) {
        std::stringstream ss;
        
        ss << "Position " << e.pos << ": " << e.message;
        mParseErrorMessage = ss.str();
        return;
    }

    mEncoding = std::make_unique<Encoding>(*node);
    NFA nfa(*node, *mEncoding);
    mDFA = std::make_unique<DFA>(nfa);    
}

bool Matcher::valid() const
{
    return (bool)mDFA;
}

const std::string &Matcher::parseErrorMessage() const
{
    return mParseErrorMessage;
}

unsigned int Matcher::match(const std::string &string) const
{
    unsigned int state = mDFA->startState();
    unsigned int matched = 0;
    for(unsigned int i=0; i<string.size(); i++) {        
        const DFA::State &currentState = mDFA->states()[state];
        Encoding::CodePoint codePoint = mEncoding->codePoint(string[i]);
        auto it = currentState.transitions.find(codePoint);
        if(it == currentState.transitions.end()) {
            break;
        } else {
            unsigned int nextState = it->second;
            if(mDFA->acceptStates().count(nextState)) {
                matched = i + 1;
            }
            state = nextState;
        }
    }

    return matched;
}