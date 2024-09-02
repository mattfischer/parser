#ifndef PARSER_IMPL_GLR_HPP
#define PARSER_IMPL_GLR_HPP

#include "Parser/Base.hpp"
#include "Parser/Tokenizer.hpp"
#include "Parser/Impl/LRTable/Multi.hpp"
#include "Util/MultiStack.hpp"

#include <span>

namespace Parser::Impl
{
    class GLRBase : public Base
    {
    public:
        GLRBase(const Grammar &grammar);

    protected:
        struct ParseStacksBase {
            virtual size_t size() = 0;
            
            virtual void pushState(size_t stack, unsigned int state) = 0;
            virtual unsigned int backState(size_t stack) = 0;
            
            virtual void eraseStack(size_t stack) = 0;
            virtual void joinStacks(size_t stack, size_t target) = 0;
        };

        void runParse(Tokenizer::Stream &stream, ParseStacksBase &stacks) const;

        virtual void shift(const Tokenizer::Token &token, unsigned int state, ParseStacksBase &stacksBase, size_t stack) const = 0;
        virtual void reduce(unsigned int rule, unsigned int rhs, ParseStacksBase &stacksBase, size_t stack, bool preserveStack) const = 0;
    
        LRTable::Multi mParseTable;
    };

    template<typename ParseData> class GLR : public GLRBase
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

        struct ParseStacks : public ParseStacksBase {
            Util::MultiStack<StackItem> stacks;

            virtual size_t size() { return stacks.size(); }
            
            virtual void pushState(size_t stack, unsigned int state) { stacks.stack(stack).push_back(StackItem{state}); }
            virtual unsigned int backState(size_t stack) { return stacks.stack(stack).back().state; }
            
            virtual void eraseStack(size_t stack) { stacks.eraseStack(stack); }
            virtual void joinStacks(size_t stack, size_t target) {
                stacks.stack(stack).pop_back();
                auto end = stacks.stack(target).end();
                auto begins = stacks.backtrack(end, 1);
                stacks.joinStack(stack, begins[0]);
            }
        };

        typedef std::function<std::shared_ptr<ParseData>(const Tokenizer::Token&)> TerminalDecorator;
        std::map<unsigned int, TerminalDecorator> mTerminalDecorators;
    
        typedef std::function<std::shared_ptr<ParseData>(ParseStackView &)> Reducer;
        std::map<unsigned int, Reducer> mReducers;

    public:
        GLR(const Grammar &grammar)
        : GLRBase(grammar)
        {
        }
    
        template<typename T> void addTerminalDecorator(const std::string &terminal, T terminalDecorator)
        {
            unsigned int terminalIndex = grammar().terminalIndex(terminal);
            if(terminalIndex != UINT_MAX) {
                mTerminalDecorators[terminalIndex] = terminalDecorator;
            }
        }

        template<typename R> void addReducer(const std::string &rule, R reducer)
        {
            unsigned int ruleIndex = grammar().ruleIndex(rule);
            if(ruleIndex != UINT_MAX) {
                mReducers[ruleIndex] = reducer;
            }
        }

        std::vector<std::shared_ptr<ParseData>> parse(Tokenizer::Stream &stream)
        {
            ParseStacks stacks;

            runParse(stream, stacks);

            std::vector<std::shared_ptr<ParseData>> results;
            for(size_t i=0; i<stacks.size(); i++) {
                results.push_back(stacks.stacks.stack(i).back().parseItems[0].data);
            }

            return results;
        }

    protected:
        virtual void shift(const Tokenizer::Token &token, unsigned int state, ParseStacksBase &stacksBase, size_t stack) const
        {
            std::shared_ptr<ParseData> data;
            auto it = mTerminalDecorators.find(token.value);
            if(it != mTerminalDecorators.end()) {
                data = it->second(token);
            }
            
            ParseItem parseItem {
                .type = ParseItem::Type::Terminal,
                .index = token.value,
                .data = data
            };
    
            StackItem stackItem {
                .state = state,
                .parseItems {std::move(parseItem)}
            };

            auto &stacks = static_cast<ParseStacks&>(stacksBase).stacks;
            stacks.stack(stack).push_back(std::move(stackItem));               
        }

        virtual void reduce(unsigned int rule, unsigned int rhs, ParseStacksBase &stacksBase, size_t stack, bool preserveStack) const
        {
            auto &stacks = static_cast<ParseStacks&>(stacksBase).stacks;

            size_t size = 0;
            for(const auto &symbol: mGrammar.rules()[rule].rhs[rhs]) {
                if(symbol.type != Grammar::Symbol::Type::Epsilon) {
                    size++;
                }
            }

            auto end = stacks.stack(stack).end();
            auto begins = stacks.backtrack(end, size + 1);
            for(size_t i = 0; i<begins.size(); i++) {
                auto &begin = begins[i];
                
                unsigned int state = mParseTable.nextStateForRule(begin->state, rule);

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

                if(i == begins.size() - 1 && !preserveStack) {
                    stacks.relocateStack(stack, begin);
                    stacks.stack(stack).push_back(stackItem);
                } else {
                    size_t newStack = stacks.addStack(begin);
                    stacks.stack(newStack).push_back(stackItem);
                }
            }
        }
    };

    template<typename ParseData> class GLR<ParseData>::ParseStackIterator {
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

    template<typename ParseData> class GLR<ParseData>::ParseStackView : public std::ranges::view_interface<ParseStackView> {
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