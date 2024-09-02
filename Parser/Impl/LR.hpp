#ifndef PARSER_IMPL_LR_HPP
#define PARSER_IMPL_LR_HPP

#include "Parser/Base.hpp"
#include "Parser/Impl/LRTable.hpp"
#include "Parser/Tokenizer.hpp"

#include <span>
#include <functional>

namespace Parser::Impl
{
    class LR : public Base
    {
    public:
        LR(const Grammar &grammar, std::unique_ptr<LRTable::Single> parseTable);

        template<typename ParseData> class ParseSession;

    private:
        std::unique_ptr<LRTable::Single> mParseTable;        
    };

    template<typename ParseData> class LR::ParseSession
    {
    public:
        struct ParseItem {
            enum class Type {
                Terminal,
                Nonterminal
            };
            Type type;
            unsigned int index;
            std::unique_ptr<ParseData> data;
        };

    private:
        const LR &mParser;

        typedef std::function<std::unique_ptr<ParseData>(const Tokenizer::Token&)> TerminalDecorator;
        std::map<unsigned int, TerminalDecorator> mTerminalDecorators;

        typedef std::function<std::unique_ptr<ParseData>(std::span<ParseItem>)> Reducer;
        std::map<unsigned int, Reducer> mReducers;

    public:
        ParseSession(const LR &parser)
        : mParser(parser)
        {
        }
    
        template <typename T> void addTerminalDecorator(const std::string &terminal, T terminalDecorator)
        {
            unsigned int terminalIndex = mParser.grammar().terminalIndex(terminal);
            if(terminalIndex != UINT_MAX) {
                mTerminalDecorators[terminalIndex] = terminalDecorator;
            }
        }

        template <typename R> void addReducer(const std::string &rule, R reducer)
        {
            unsigned int ruleIndex = mParser.grammar().ruleIndex(rule);
            if(ruleIndex != UINT_MAX) {
                mReducers[ruleIndex] = reducer;
            }
        }

        std::unique_ptr<ParseData> parse(Tokenizer::Stream &stream)
        {
            struct StateItem {
                unsigned int state;
                unsigned int parseStackStart;
            };

            std::vector<StateItem> stateStack;
            std::vector<ParseItem> parseStack;
            unsigned int state = 0;

            while(!mParser.mParseTable->isAccept(state)) {
                stateStack.push_back(StateItem{state, (unsigned int)parseStack.size()});
                mParser.mParseTable->process(state, stream.nextToken().value,
                    [&](unsigned int newState) {
                        ParseItem parseItem;
                        parseItem.type = ParseItem::Type::Terminal;
                        parseItem.index = stream.nextToken().value;
                        auto it = mTerminalDecorators.find(stream.nextToken().value);
                        if(it != mTerminalDecorators.end()) {
                            parseItem.data = it->second(stream.nextToken());
                        }
                        parseStack.push_back(std::move(parseItem));

                        stream.consumeToken();
                        state = newState;
                    },
                    [&](unsigned int rule, unsigned int rhs) {
                        const Grammar::RHS &ruleRhs = mParser.grammar().rules()[rule].rhs[rhs];
                        for(unsigned int i=0; i<ruleRhs.size(); i++) {
                            if(ruleRhs[i].type != Grammar::Symbol::Type::Epsilon) {
                                stateStack.pop_back();
                            }
                        }

                        state = stateStack.back().state;
                        unsigned int parseStackStart = stateStack.back().parseStackStart;

                        auto it = mReducers.find(rule);
                        if(it != mReducers.end()) {
                            std::span<ParseItem> span(&parseStack[parseStackStart], &parseStack[0] + parseStack.size());
                            std::unique_ptr<ParseData> data = it->second(span);
                            parseStack.erase(parseStack.begin() + parseStackStart, parseStack.end());

                            ParseItem parseItem {
                                .type = ParseItem::Type::Nonterminal,
                                .index = rule,
                                .data = std::move(data)
                            };
                            parseStack.push_back(std::move(parseItem));
                        }
                        
                        state = mParser.mParseTable->nextStateForRule(state, rule); 
                    },
                    [&]() {
                        return std::unique_ptr<ParseData>();
                    }
                );
            }

            std::unique_ptr<ParseData> result;
            auto it = mReducers.find(mParser.grammar().startRule());
            if(it != mReducers.end()) {
                std::span<ParseItem> span(&parseStack[0], &parseStack[0] + parseStack.size());
                result = it->second(span);
            }

            return std::move(result);
        }
    };
}
#endif