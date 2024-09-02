#include "Parser/Impl/LR.hpp"

namespace Parser::Impl
{
    LRBase::LRBase(const Grammar &grammar, std::unique_ptr<LRTable::Single> parseTable)
    : Base(grammar)
    , mParseTable(std::move(parseTable))
    {
    }

    bool LRBase::runParse(Tokenizer::Stream &stream, ParseStackBase &parseStack) const
    {
        struct StateItem {
            unsigned int state;
            unsigned int parseStackStart;
        };

        std::vector<StateItem> stateStack;
        unsigned int state = 0;

        while(!mParseTable->isAccept(state)) {
            stateStack.push_back(StateItem{state, (unsigned int)parseStack.size()});
            mParseTable->process(state, stream.nextToken().value,
                [&](unsigned int newState) {
                    shift(stream.nextToken(), parseStack);
                    stream.consumeToken();
                    state = newState;
                },
                [&](unsigned int rule, unsigned int rhs) {
                    const Grammar::RHS &ruleRhs = grammar().rules()[rule].rhs[rhs];
                    for(unsigned int i=0; i<ruleRhs.size(); i++) {
                        if(ruleRhs[i].type != Grammar::Symbol::Type::Epsilon) {
                            stateStack.pop_back();
                        }
                    }

                    state = stateStack.back().state;
                    unsigned int parseStackStart = stateStack.back().parseStackStart;

                    reduce(rule, parseStackStart, parseStack);
                    
                    state = mParseTable->nextStateForRule(state, rule); 
                },
                [&]() {
                    return false;
                }
            );
        }

        reduce(grammar().startRule(), 0, parseStack);
        return true;
    }
}