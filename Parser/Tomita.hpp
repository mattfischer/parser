#ifndef PARSER_TOMITA_HPP
#define PARSER_TOMITA_HPP

#include "Parser/LRMulti.hpp"
#include "Parser/MultiStack.hpp"

namespace Parser
{
    class Tomita : public LRMulti
    {
    public:
        Tomita(const Grammar &grammar);

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
            
            ParseSession(const Tomita &parser)
            : mParser(parser)
            {
            }
        
            void addTerminalDecorator(const std::string &terminal, TerminalDecorator terminalDecorator)
            {
                unsigned int terminalIndex = mParser.grammar().terminalIndex(terminal);
                if(terminalIndex != UINT_MAX) {
                    mTerminalDecorators[terminalIndex] = terminalDecorator;
                }
            }

            void addReducer(const std::string &rule, Reducer reducer)
            {
                unsigned int ruleIndex = mParser.grammar().ruleIndex(rule);
                if(ruleIndex != UINT_MAX) {
                    mReducers[ruleIndex] = reducer;
                }
            }

            std::vector<std::shared_ptr<ParseData>> parse(Tokenizer::Stream &stream)
            {
                struct StackItem {
                    unsigned int state;
                    ParseItem<ParseData> parseItem;
                    std::vector<ParseItem<ParseData>> unreduced;
                };

                MultiStack<StackItem> stacks;

                stacks.push_back(0, StackItem{0});

                auto reduce = [&](size_t stack, unsigned int rule, unsigned int rhs, bool allowReplace) {
                    size_t size = mParser.mGrammar.rules()[rule].rhs[rhs].size();
                    std::vector<MultiStack<StackItem>::iterator> begins;
                    MultiStack<StackItem>::iterator end;
                    stacks.enumerate(stack, size + 1, begins, end);
                    for(size_t i = 0; i<begins.size(); i++) {
                        auto &begin = begins[i];
                        std::vector<ParseItem<ParseData>> parseStack;
                        parseStack.reserve(size);
                        
                        unsigned int state = begin->state;
                        const ParseTableEntry &newEntry = mParser.mParseTable.at(state, mParser.ruleIndex(rule));
                        state = newEntry.index;
            
                        ++begin;
                        for(auto it = begin; it != end; ++it) {
                            if(it->unreduced.size() > 0) {
                                parseStack.insert(parseStack.end(), it->unreduced.begin(), it->unreduced.end());
                            } else {
                                parseStack.push_back(it->parseItem);
                            }
                        }

                        StackItem stackItem;
                        stackItem.state = state;

                        auto it = mReducers.find(rule);
                        if(it == mReducers.end()) {
                            stackItem.unreduced = std::move(parseStack);
                        } else {
                            std::shared_ptr<ParseData> data = it->second(&parseStack[0], (unsigned int)size);
                            stackItem.parseItem = ParseItem<ParseData>{ParseItem<ParseData>::Type::Nonterminal, rule, data};
                        }

                        if(i == begins.size() - 1 && allowReplace) {
                            stacks.replace(stack, begin, stackItem);
                        } else {
                            stacks.add(begin, stackItem);
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
                                            bool allowReplace = false;
                                            if(j == entries.size() - 1) {
                                                allowReplace = true;
                                                repeat = true;
                                            }
                                            reduce(i, reduction.rule, reduction.rhs, allowReplace);
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

                    // TODO: Merge stacks
                }

                std::vector<std::shared_ptr<ParseData>> results;
                for(size_t i=0; i<stacks.size(); i++) {
                    reduce(i, mParser.mGrammar.startRule(), 0, true);
                    results.push_back(stacks.back(i).parseItem.data);
                }

                return results;
            }

        private:
            const Tomita &mParser;
            std::map<unsigned int, TerminalDecorator> mTerminalDecorators;
            std::map<unsigned int, Reducer> mReducers;
        };
    };
}
#endif