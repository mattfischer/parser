#include "Matcher.hpp"

#include "NFA.hpp"

namespace Regex {

    Matcher::Matcher(const std::vector<std::string> &patterns)
    {
        mNumPatterns = (unsigned int)patterns.size();

        std::vector<std::unique_ptr<Parser::Node>> nodes;
        
        try {
            for(unsigned int i=0; i<patterns.size(); i++) {
                const std::string &pattern = patterns[i];
                mParseError.pattern = i;
                nodes.push_back(Parser::parse(pattern));
            }
        } catch(Parser::ParseException e) {
            mParseError.character = e.pos;
            mParseError.message = e.message;
            return;
        }

        mParseError.pattern = 0;
        mParseError.character = 0;

        mEncoding = std::make_unique<Encoding>(nodes);
        NFA nfa(nodes, *mEncoding);
        mDFA = std::make_unique<DFA>(nfa, *mEncoding);
    }

    bool Matcher::valid() const
    {
        return (bool)mDFA;
    }

    const Matcher::ParseError &Matcher::parseError() const
    {
        return mParseError;
    }

    unsigned int Matcher::match(const std::string &string, unsigned int start, unsigned int &pattern) const
    {
        unsigned int state = mDFA->startState();
        unsigned int matched = 0;
        
        for(unsigned int i=start; i<string.size(); i++) {    
            Encoding::CodePoint codePoint = mEncoding->codePoint(string[i]);
            if(codePoint == Encoding::kInvalidCodePoint) {
                break;
            }
            unsigned int nextState = mDFA->transition(state, codePoint);
            
            if(nextState == mDFA->rejectState()) {
                break;
            } else {
                if(mDFA->accept(nextState, pattern)) {
                    matched = (i - start) + 1;
                }
                state = nextState;
            }
        }

        return matched;
    }

    unsigned int Matcher::numPatterns() const
    {
        return mNumPatterns;
    }
}