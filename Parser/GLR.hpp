#ifndef PARSER_GLR_HPP
#define PARSER_GLR_HPP

#include "Parser/LRMulti.hpp"
#include "Parser/MultiStack.hpp"

namespace Parser
{
    class GLR : public LRMulti
    {
    public:
        GLR(const Grammar &grammar);

        template<typename ParseData> class ParseSession
        {
        public:
            struct ParseItem {
                enum class Type {
                    Terminal,
                    Nonterminal
                };
                Type type;
                unsigned int index;
                std::shared_ptr<ParseData> data;

                typedef ParseItem* iterator;
            };

            typedef std::function<std::shared_ptr<ParseData>(const Tokenizer::Token&)> TerminalDecorator;
            typedef std::function<std::shared_ptr<ParseData>(typename ParseItem::iterator, typename ParseItem::iterator)> Reducer;
            
            ParseSession(const GLR &parser);
        
            void addTerminalDecorator(const std::string &terminal, TerminalDecorator terminalDecorator);
            void addReducer(const std::string &rule, Reducer reducer);

            std::vector<std::shared_ptr<ParseData>> parse(Tokenizer::Stream &stream);

        private:
            struct StackItem {
                unsigned int state;
                std::vector<ParseItem> parseItems;
            };

            void reduce(MultiStack<StackItem> &stacks, size_t stack, unsigned int rule, unsigned int rhs, bool allowRelocate);

            const GLR &mParser;
            std::map<unsigned int, TerminalDecorator> mTerminalDecorators;
            std::map<unsigned int, Reducer> mReducers;
        };
    };

    template<typename ParseData> GLR::ParseSession<ParseData>::ParseSession(const GLR &parser)
    : mParser(parser)
    {
    }

    template<typename ParseData> void GLR::ParseSession<ParseData>::addTerminalDecorator(const std::string &terminal, TerminalDecorator terminalDecorator)
    {
        unsigned int terminalIndex = mParser.grammar().terminalIndex(terminal);
        if(terminalIndex != UINT_MAX) {
            mTerminalDecorators[terminalIndex] = terminalDecorator;
        }
    }

    template<typename ParseData> void GLR::ParseSession<ParseData>::addReducer(const std::string &rule, Reducer reducer)
    {
        unsigned int ruleIndex = mParser.grammar().ruleIndex(rule);
        if(ruleIndex != UINT_MAX) {
            mReducers[ruleIndex] = reducer;
        }
    }

    template<typename ParseData> std::vector<std::shared_ptr<ParseData>> GLR::ParseSession<ParseData>::parse(Tokenizer::Stream &stream)
    {
        MultiStack<StackItem> stacks;
        stacks.push_back(0, StackItem{0});    

        while(true) {
            std::shared_ptr<ParseData> terminal;
            auto it = mTerminalDecorators.find(stream.nextToken().value);
            if(it != mTerminalDecorators.end()) {
                terminal = it->second(stream.nextToken());
            }

            bool repeat = false;
            for(size_t i=0; i<stacks.size() || repeat; i++) {
                if(repeat) {
                    i--;
                    repeat = false;
                    if(i >= stacks.size()) {
                        break;
                    }
                }

                unsigned int state = stacks.back(i).state;
                if(mParser.mAcceptStates.count(state) > 0) {
                    continue;
                }

                const ParseTableEntry &entry = mParser.mParseTable.at(state, stream.nextToken().value);
                switch(entry.type) {
                    case ParseTableEntry::Type::Shift:
                    {
                        ParseItem parseItem{ParseItem::Type::Terminal, stream.nextToken().value, terminal};
                        StackItem stackItem;
                        stackItem.state = entry.index;
                        stackItem.parseItems.push_back(std::move(parseItem));
                        stacks.push_back(i, std::move(stackItem));
                        break;
                    }

                    case ParseTableEntry::Type::Reduce:
                    {
                        const Reduction &reduction = mParser.mReductions[entry.index];
                        reduce(stacks, i, reduction.rule, reduction.rhs, true);
                        repeat = true;
                        break;
                    }

                    case ParseTableEntry::Type::Multi:
                    {
                        const auto &entries = mParser.mMultiEntries[entry.index];
                        for(size_t j=0; j<entries.size(); j++) {
                            const auto &entry = entries[j];
                            switch(entry.type) {
                                case ParseTableEntry::Type::Reduce:
                                {
                                    const Reduction &reduction = mParser.mReductions[entry.index];
                                    bool allowRelocate = false;
                                    if(j == entries.size() - 1) {
                                        allowRelocate = true;
                                        repeat = true;
                                    }
                                    reduce(stacks, i, reduction.rule, reduction.rhs, allowRelocate);
                                    break;
                                }

                                case ParseTableEntry::Type::Shift:
                                {
                                    ParseItem parseItem{ParseItem::Type::Terminal, stream.nextToken().value, terminal};
                                    StackItem stackItem;
                                    stackItem.state = entry.index;
                                    stackItem.parseItems.push_back(std::move(parseItem));
                                    stacks.push_back(i, std::move(stackItem));
                                    break;
                                }

                                default:
                                    break;
                            }
                        }
                        break;
                    }

                    case ParseTableEntry::Type::Error:
                    {
                        stacks.erase(i);
                        repeat = true;
                        break;
                    }
                }
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
                    const StackItem &stackItem = stacks.back(i);
                    auto it = stackMap.find(stackItem.state);
                    if(it == stackMap.end()) {
                        stackMap[stackItem.state] = i;
                    } else {
                        stacks.pop_back(i);
                        std::vector<MultiStack<StackItem>::iterator> begins = stacks.backtrack(stacks.end(it->second), 1);
                        stacks.join(i, begins[0]);
                        repeat = true;
                    }
                }
            }
        }

        std::vector<std::shared_ptr<ParseData>> results;
        for(size_t i=0; i<stacks.size(); i++) {
            reduce(stacks, i, mParser.mGrammar.startRule(), 0, true);
            results.push_back(stacks.back(i).parseItems[0].data);
        }

        return results;
    }

    template<typename ParseData> void GLR::ParseSession<ParseData>::reduce(MultiStack<StackItem> &stacks, size_t stack, unsigned int rule, unsigned int rhs, bool allowRelocate)
    {
        size_t size = 0;
        for(const auto &symbol: mParser.mGrammar.rules()[rule].rhs[rhs]) {
            if(symbol.type != Grammar::Symbol::Type::Epsilon) {
                size++;
            }
        }

        MultiStack<StackItem>::Locator end = stacks.end(stack);
        std::vector<MultiStack<StackItem>::iterator> begins = stacks.backtrack(end, size + 1);
        for(size_t i = 0; i<begins.size(); i++) {
            auto &begin = begins[i];
            std::vector<ParseItem> parseStack;
            parseStack.reserve(size);
            
            unsigned int state = begin->state;
            const ParseTableEntry &newEntry = mParser.mParseTable.at(state, mParser.ruleIndex(rule));
            state = newEntry.index;

            ++begin;
            for(auto it = begin; it != end; ++it) {
                parseStack.insert(parseStack.end(), it->parseItems.begin(), it->parseItems.end());
            }

            StackItem stackItem;
            stackItem.state = state;

            auto it = mReducers.find(rule);
            if(it == mReducers.end()) {
                stackItem.parseItems = std::move(parseStack);
            } else {
                std::shared_ptr<ParseData> data = it->second(&parseStack[0], &parseStack[0] + parseStack.size());
                stackItem.parseItems.push_back(ParseItem{ParseItem::Type::Nonterminal, rule, data});
            }

            if(i == begins.size() - 1 && allowRelocate) {
                stacks.relocate(stack, begin);
                stacks.push_back(stack, stackItem);
            } else {
                size_t newStack = stacks.add(begin);
                stacks.push_back(newStack, stackItem);
            }
        }
    }
}
#endif