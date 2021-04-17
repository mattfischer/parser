#include "Matcher.hpp"

#include "NFA.hpp"

#include <sstream>

namespace Regex {

    Matcher::Matcher(const std::vector<std::string> &patterns)
    {
        std::vector<std::unique_ptr<Parser::Node>> nodes;
        
        try {
            for(const auto &pattern : patterns) {
                nodes.push_back(Parser::parse(pattern));
            }
        } catch(Parser::ParseException e) {
            std::stringstream ss;
            
            ss << "Position " << e.pos << ": " << e.message;
            mParseErrorMessage = ss.str();
            return;
        }

        mEncoding = std::make_unique<Encoding>(nodes);
        NFA nfa(nodes, *mEncoding);
        mDFA = std::make_unique<DFA>(nfa, *mEncoding);
    }

    bool Matcher::valid() const
    {
        return (bool)mDFA;
    }

    const std::string &Matcher::parseErrorMessage() const
    {
        return mParseErrorMessage;
    }

    unsigned int Matcher::match(const std::string &string, unsigned int &pattern) const
    {
        unsigned int state = mDFA->startState();
        unsigned int matched = 0;
        
        for(unsigned int i=0; i<string.size(); i++) {    
            Encoding::CodePoint codePoint = mEncoding->codePoint(string[i]);
            unsigned int nextState = mDFA->transition(state, codePoint);
            
            if(nextState == mDFA->rejectState()) {
                break;
            } else {
                if(mDFA->accept(nextState, pattern)) {
                    matched = i + 1;
                }
                state = nextState;
            }
        }

        return matched;
    }
}