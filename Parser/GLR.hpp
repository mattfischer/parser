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

        template<typename Data> struct ParseItem {
            enum class Type {
                Terminal,
                Nonterminal
            };
            Type type;
            unsigned int index;
            std::shared_ptr<Data> data;
        };

        template<typename ParseData> class ParseSession
        {
        public:
            typedef std::function<std::shared_ptr<ParseData>(const Tokenizer::Token&)> TerminalDecorator;
            typedef std::function<std::shared_ptr<ParseData>(ParseItem<ParseData>*, unsigned int)> Reducer;
            
            ParseSession(const GLR &parser);
        
            void addTerminalDecorator(const std::string &terminal, TerminalDecorator terminalDecorator);
            void addReducer(const std::string &rule, Reducer reducer);

            std::vector<std::shared_ptr<ParseData>> parse(Tokenizer::Stream &stream);

        private:
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
        struct ParseItems {
            std::vector<ParseItem<ParseData>> items;
        };
        struct StackItem {
            unsigned int state;
            ParseItem<ParseData> parseItem;
            std::shared_ptr<ParseItems> unreduced;
        };

        MultiStack<StackItem> stacks;

        stacks.push_back(0, StackItem{0});

        auto reduce = [&](size_t stack, unsigned int rule, unsigned int rhs, bool allowRelocate) {
            size_t size = 0;
            for(const auto &symbol: mParser.mGrammar.rules()[rule].rhs[rhs]) {
                if(symbol.type != Grammar::Symbol::Type::Epsilon) {
                    size++;
                }
            }

            std::vector<MultiStack<StackItem>::iterator> begins = stacks.enumerate(stack, size + 1);
            MultiStack<StackItem>::iterator end = stacks.end(stack);
            for(size_t i = 0; i<begins.size(); i++) {
                auto &begin = begins[i];
                std::vector<ParseItem<ParseData>> parseStack;
                parseStack.reserve(size);
                
                unsigned int state = begin->state;
                const ParseTableEntry &newEntry = mParser.mParseTable.at(state, mParser.ruleIndex(rule));
                state = newEntry.index;
    
                ++begin;
                for(auto it = begin; it != end; ++it) {
                    if(it->unreduced) {
                        parseStack.insert(parseStack.end(), it->unreduced->items.begin(), it->unreduced->items.end());
                    } else {
                        parseStack.push_back(it->parseItem);
                    }
                }

                StackItem stackItem;
                stackItem.state = state;

                auto it = mReducers.find(rule);
                if(it == mReducers.end()) {
                    stackItem.unreduced = std::make_shared<ParseItems>();
                    stackItem.unreduced->items = std::move(parseStack);
                } else {
                    std::shared_ptr<ParseData> data = it->second(&parseStack[0], (unsigned int)parseStack.size());
                    stackItem.parseItem = ParseItem<ParseData>{ParseItem<ParseData>::Type::Nonterminal, rule, data};
                }

                if(i == begins.size() - 1 && allowRelocate) {
                    stacks.relocate(stack, begin);
                    stacks.push_back(stack, stackItem);
                } else {
                    size_t newStack = stacks.add(begin);
                    stacks.push_back(newStack, stackItem);
                }
            }
        };

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
                        ParseItem<ParseData> parseItem{ParseItem<ParseData>::Type::Terminal, stream.nextToken().value, terminal};
                        stacks.push_back(i, StackItem{entry.index, std::move(parseItem)});
                        break;
                    }

                    case ParseTableEntry::Type::Reduce:
                    {
                        const Reduction &reduction = mParser.mReductions[entry.index];
                        reduce(i, reduction.rule, reduction.rhs, true);
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
                                    reduce(i, reduction.rule, reduction.rhs, allowRelocate);
                                    break;
                                }

                                case ParseTableEntry::Type::Shift:
                                {
                                    ParseItem<ParseData> parseItem{ParseItem<ParseData>::Type::Terminal, stream.nextToken().value, terminal};
                                    stacks.push_back(i, StackItem{entry.index, std::move(parseItem)});
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
                        std::vector<MultiStack<StackItem>::iterator> begins = stacks.enumerate(it->second, 1);
                        stacks.join(i, begins[0]);
                        repeat = true;
                    }
                }
            }
        }

        std::vector<std::shared_ptr<ParseData>> results;
        for(size_t i=0; i<stacks.size(); i++) {
            reduce(i, mParser.mGrammar.startRule(), 0, true);
            results.push_back(stacks.back(i).parseItem.data);
        }

        return results;
    }
}
#endif