#ifndef PARSER_IMPL_LR_HPP
#define PARSER_IMPL_LR_HPP

#include "Parser/Impl/Base.hpp"
#include "Parser/Impl/LRTable/Single.hpp"
#include "Parser/Tokenizer.hpp"

#include <span>
#include <functional>

namespace Parser::Impl
{
    class LRBase : public Base
    {
    public:
        LRBase(const Grammar &grammar, std::unique_ptr<LRTable::Single> parseTable);

        bool valid() const;

        using Conflict = LRTable::Single::Conflict;
        const Conflict &conflict() const;

    protected:
        struct ParseStackBase {
            virtual size_t size() = 0;
        };

        bool runParse(Tokenizer::Stream &stream, ParseStackBase &parseStack) const;

        virtual void shift(const Tokenizer::Token &token, ParseStackBase &stackBase) const = 0;
        virtual void reduce(unsigned int rule, unsigned int stackStart, ParseStackBase &stackBase) const = 0;

        std::unique_ptr<LRTable::Single> mParseTable;        
    };

    template<typename ParseData, typename ParseTable> class LR : public LRBase
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
        typedef std::function<std::unique_ptr<ParseData>(const Tokenizer::Token&)> TerminalDecorator;
        std::map<unsigned int, TerminalDecorator> mTerminalDecorators;

        typedef std::function<std::unique_ptr<ParseData>(std::span<ParseItem>)> Reducer;
        std::map<unsigned int, Reducer> mReducers;

        struct ParseStack : public ParseStackBase {
            std::vector<ParseItem> items;

            virtual size_t size() { return items.size(); }
        };

    public:
        LR(const Grammar &grammar)
        : LRBase(grammar, std::make_unique<ParseTable>(grammar))
        {
        }
    
        template <typename T> void addTerminalDecorator(const std::string &terminal, T terminalDecorator)
        {
            unsigned int terminalIndex = grammar().terminalIndex(terminal);
            if(terminalIndex != UINT_MAX) {
                mTerminalDecorators[terminalIndex] = terminalDecorator;
            }
        }

        template <typename R> void addReducer(const std::string &rule, R reducer)
        {
            unsigned int ruleIndex = grammar().ruleIndex(rule);
            if(ruleIndex != UINT_MAX) {
                mReducers[ruleIndex] = reducer;
            }
        }

        std::unique_ptr<ParseData> parse(Tokenizer::Stream &stream) const
        {
            ParseStack stack;
            if(runParse(stream, stack)) {
                return std::move(stack.items[0].data);
            } else {
                return std::unique_ptr<ParseData>();
            }
        }

    protected:
        virtual void shift(const Tokenizer::Token &token, ParseStackBase &stackBase) const
        {
            ParseStack &stack = static_cast<ParseStack&>(stackBase);

            std::unique_ptr<ParseData> data;
            auto it = mTerminalDecorators.find(token.value);
            if(it != mTerminalDecorators.end()) {
                data = it->second(token);
            }

            ParseItem parseItem {
                .type = ParseItem::Type::Terminal,
                .index = token.value,
                .data = std::move(data)
            };
            
            stack.items.push_back(std::move(parseItem));
        }

        virtual void reduce(unsigned int rule, unsigned int stackStart, ParseStackBase &stackBase) const
        {
            ParseStack &stack = static_cast<ParseStack&>(stackBase);
 
            auto it = mReducers.find(rule);
            if(it != mReducers.end()) {
                std::span<ParseItem> span(&stack.items[stackStart], &stack.items[0] + stack.items.size());
                std::unique_ptr<ParseData> data = it->second(span);
                stack.items.erase(stack.items.begin() + stackStart, stack.items.end());

                ParseItem parseItem {
                    .type = ParseItem::Type::Nonterminal,
                    .index = rule,
                    .data = std::move(data)
                };
                stack.items.push_back(std::move(parseItem));
            }
        }
    };
}
#endif