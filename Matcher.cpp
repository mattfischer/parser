#include "Matcher.hpp"

Matcher::Matcher(const DFA &dfa, const Encoding &encoding)
: mDFA(dfa), mEncoding(encoding)
{
}

unsigned int Matcher::match(const std::string &string) const
{
    unsigned int state = mDFA.startState();
    unsigned int matched = 0;
    for(unsigned int i=0; i<string.size(); i++) {        
        const DFA::State &currentState = mDFA.states()[state];
        Encoding::CodePoint codePoint = mEncoding.codePoint(string[i]);
        auto it = currentState.transitions.find(codePoint);
        if(it == currentState.transitions.end()) {
            break;
        } else {
            unsigned int nextState = it->second;
            if(mDFA.acceptStates().count(nextState)) {
                matched = i + 1;
            }
            state = nextState;
        }
    }

    return matched;
}