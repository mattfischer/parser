#ifndef PARSER_IMPL_GLR_HPP
#define PARSER_IMPL_GLR_HPP

#include "Parser/Impl/LRMulti.hpp"
#include "Util/MultiStack.hpp"

#include <span>

namespace Parser::Impl
{
    class GLR : public LRMulti
    {
    public:
        GLR(const Grammar &grammar);

        template<typename ParseData> class ParseSession;
    };

    template<typename ParseData> class GLR::ParseSession
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
        };
        
        class ParseStackIterator;
        class ParseStackView;

    private:
        struct StackItem {
            unsigned int state;
            std::vector<ParseItem> parseItems;
        };

        const GLR &mParser;

        typedef std::function<std::shared_ptr<ParseData>(const Tokenizer::Token&)> TerminalDecorator;
        std::map<unsigned int, TerminalDecorator> mTerminalDecorators;
    
        typedef std::function<std::shared_ptr<ParseData>(ParseStackView &)> Reducer;
        std::map<unsigned int, Reducer> mReducers;

    public:
        ParseSession(const GLR &parser)
        : mParser(parser)
        {
        }
    
        template<typename T> void addTerminalDecorator(const std::string &terminal, T terminalDecorator)
        {
            unsigned int terminalIndex = mParser.grammar().terminalIndex(terminal);
            if(terminalIndex != UINT_MAX) {
                mTerminalDecorators[terminalIndex] = terminalDecorator;
            }
        }

        template<typename R> void addReducer(const std::string &rule, R reducer)
        {
            unsigned int ruleIndex = mParser.grammar().ruleIndex(rule);
            if(ruleIndex != UINT_MAX) {
                mReducers[ruleIndex] = reducer;
            }
        }

        std::vector<std::shared_ptr<ParseData>> parse(Tokenizer::Stream &stream)
        {
            Util::MultiStack<StackItem> stacks;
            stacks.stack(0).push_back(StackItem{0});    

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

                    unsigned int state = stacks.stack(i).back().state;
                    if(mParser.mAcceptStates.contains(state)) {
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
                            stacks.stack(i).push_back(std::move(stackItem));
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
                                        stacks.stack(i).push_back(std::move(stackItem));
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
                            stacks.eraseStack(i);
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
                        const StackItem &stackItem = stacks.stack(i).back();
                        auto it = stackMap.find(stackItem.state);
                        if(it == stackMap.end()) {
                            stackMap[stackItem.state] = i;
                        } else {
                            stacks.stack(i).pop_back();
                            typename Util::MultiStack<StackItem>::Locator end = stacks.stack(it->second).end();
                            std::vector<typename Util::MultiStack<StackItem>::PathIterator> begins = stacks.backtrack(end, 1);
                            stacks.joinStack(i, begins[0]);
                            repeat = true;
                        }
                    }
                }
            }

            std::vector<std::shared_ptr<ParseData>> results;
            for(size_t i=0; i<stacks.size(); i++) {
                reduce(stacks, i, mParser.mGrammar.startRule(), 0, true);
                results.push_back(stacks.stack(i).back().parseItems[0].data);
            }

            return results;
        }

    private:
        void reduce(Util::MultiStack<StackItem> &stacks, size_t stack, unsigned int rule, unsigned int rhs, bool allowRelocate)
        {
            size_t size = 0;
            for(const auto &symbol: mParser.mGrammar.rules()[rule].rhs[rhs]) {
                if(symbol.type != Grammar::Symbol::Type::Epsilon) {
                    size++;
                }
            }

            typename Util::MultiStack<StackItem>::Locator end = stacks.stack(stack).end();
            std::vector<typename Util::MultiStack<StackItem>::PathIterator> begins = stacks.backtrack(end, size + 1);
            for(size_t i = 0; i<begins.size(); i++) {
                auto &begin = begins[i];
                
                unsigned int state = begin->state;
                const ParseTableEntry &newEntry = mParser.mParseTable.at(state, mParser.ruleIndex(rule));
                state = newEntry.index;

                ++begin;
                ParseStackView view(begin, end);
                
                StackItem stackItem;
                stackItem.state = state;

                auto it = mReducers.find(rule);
                if(it == mReducers.end()) {
                    for(auto i : view) {
                        stackItem.parseItems.push_back(i);
                    }
                } else {
                    std::shared_ptr<ParseData> data = it->second(view);
                    stackItem.parseItems.push_back(ParseItem{ParseItem::Type::Nonterminal, rule, data});
                }

                if(i == begins.size() - 1 && allowRelocate) {
                    stacks.relocateStack(stack, begin);
                    stacks.stack(stack).push_back(stackItem);
                } else {
                    size_t newStack = stacks.addStack(begin);
                    stacks.stack(newStack).push_back(stackItem);
                }
            }
        }
    };

    template<typename ParseData> class GLR::ParseSession<ParseData>::ParseStackIterator {
    private:
        typename Util::MultiStack<StackItem>::PathIterator mPathIterator;
        typename std::vector<ParseItem>::iterator mItemIterator;
        bool mResetItem;

    public:
        ParseStackIterator(Util::MultiStack<StackItem>::PathIterator pathIterator)
        : mPathIterator(pathIterator)
        {
            mResetItem = true;
        }

        ParseItem &operator*()
        {
            checkResetItem();
            return *mItemIterator;
        }

        ParseItem *operator->()
        {
            checkResetItem();
            return &(*mItemIterator);
        }

        ParseStackIterator &operator++()
        {
            ++mItemIterator;
            if(mItemIterator == mPathIterator->parseItems.end()) {
                ++mPathIterator;
                mResetItem = true;
            }

            return *this;
        }

        bool operator==(Util::MultiStack<StackItem>::Locator locator)
        {
            return mPathIterator == locator;
        }

        bool operator!=(Util::MultiStack<StackItem>::Locator locator)
        {
            return mPathIterator != locator;
        }

    private:
        void checkResetItem()
        {
            if(mResetItem) {
                mItemIterator = mPathIterator->parseItems.begin();
                mResetItem = false;
            }
        }
    };

    template<typename ParseData> class GLR::ParseSession<ParseData>::ParseStackView : public std::ranges::view_interface<ParseStackView> {
    private:
        ParseStackIterator mBegin;
        Util::MultiStack<StackItem>::Locator mEnd;

    public:
        ParseStackView(ParseStackIterator begin, Util::MultiStack<StackItem>::Locator end)
        : mBegin(std::move(begin)), mEnd(std::move(end))
        {
        }

        ParseStackIterator &begin()
        {
            return mBegin;
        }

        Util::MultiStack<StackItem>::Locator &end()
        {
            return mEnd;
        }
    };
}
#endif