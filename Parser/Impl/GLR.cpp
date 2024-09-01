#include "Parser/Impl/GLR.hpp"

namespace Parser::Impl
{
    GLRBase::GLRBase(const Grammar &grammar)
    : Base(grammar)
    , mParseTable(grammar)
    {
    }

    void GLRBase::runParse(Tokenizer::Stream &stream, ParseStacksBase &stacks) const
    {
        stacks.pushState(0, 0);

        while(true) {
            bool repeat = false;
            for(size_t i=0; i<stacks.size() || repeat; i++) {
                if(repeat) {
                    i--;
                    repeat = false;
                    if(i >= stacks.size()) {
                        break;
                    }
                }

                unsigned int state = stacks.backState(i);
                if(mParseTable.isAccept(state)) {
                    continue;
                }

                mParseTable.process(state, stream.nextToken().value, 
                    [&](unsigned int newState) {
                        shift(stream.nextToken(), newState, stacks, i);
                    },
                    [&](unsigned int rule, unsigned int rhs, bool final) {
                        reduce(rule, rhs, stacks, i, !final);
                        if(final) {
                            repeat = true; 
                        }
                    },
                    [&]() {
                        stacks.eraseStack(i);
                        repeat = true;
                    }
                );
            }

            if(stream.nextToken().value == stream.tokenizer().endValue()) {
                break;
            }
            stream.consumeToken();

            if(stacks.size() > 1) {
                std::map<unsigned int, size_t> stackMap;
                repeat = false;
                for(size_t i=0; i<stacks.size() || repeat; i++) {
                    if(repeat) {
                        i--;
                        repeat = false;
                        if(i >= stacks.size()) {
                            break;
                        }
                    }

                    unsigned int state = stacks.backState(i);
                    auto it = stackMap.find(state);
                    if(it == stackMap.end()) {
                        stackMap[state] = i;
                    } else {
                        stacks.joinStacks(i, it->second);
                        repeat = true;
                    }
                }
            }
        }

        for(size_t i=0; i<stacks.size(); i++) {
            reduce(grammar().startRule(), 0, stacks, i, false);
        }
    }
}