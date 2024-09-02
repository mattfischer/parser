#ifndef PARSER_IMPL_EARLEY_HPP
#define PARSER_IMPL_EARLEY_HPP

#include "Parser/Impl/Base.hpp"
#include "Parser/Tokenizer.hpp"

#include "Util/MultiStack.hpp"

#include <set>
#include <vector>
#include <algorithm>
#include <functional>

namespace Parser::Impl
{
    class EarleyBase : public Base
    {
    public:
        EarleyBase(const Grammar &grammar);

        struct Item {
            unsigned int rule;
            unsigned int rhs;
            unsigned int pos;
            unsigned int start;

            bool operator<(const Item &other) const;
        };

        template<typename ParseData> class ParseSession;
        
    protected:
        std::vector<Item> predict(unsigned int ruleIndex, unsigned int pos) const;
        std::vector<Item> scan(std::set<Item> &items, const Grammar::Symbol &symbol) const;    
        void populateSets(std::vector<Item> &items, std::vector<std::set<Item>> &active, std::vector<std::set<Item>> &completed, unsigned int pos) const;
        std::vector<unsigned int> findStarts(const std::vector<std::set<Item>> &completedSets, const std::vector<unsigned int> &terminalIndices, const Grammar::Symbol &symbol, unsigned int end, unsigned int minStart) const;
        std::vector<std::vector<unsigned int>> findPartitions(const std::vector<std::set<Item>> &completedSets, const std::vector<unsigned int> &terminalIndices, unsigned int rule, unsigned int rhs, unsigned int start, unsigned int end) const;

        void printSets(const std::vector<std::set<Item>> &active, const std::vector<std::set<Item>> &completed) const;
        void printSets(const std::vector<std::set<Item>> &completed) const;
        void printItem(const Item &item) const;

        typedef std::function<void(const Tokenizer::Token&)> TokenListener;
        std::vector<std::set<Item>> computeSets(Tokenizer::Stream &stream, TokenListener tokenListener) const;
    };

    template<typename ParseData> class Earley : public EarleyBase
    {
    public:
        struct ParseItem {
            enum class Type {
                Terminal,
                Nonterminal,
            };
            Type type;
            unsigned int index;
            std::shared_ptr<ParseData> data;
        };

    private:
        typedef std::function<std::shared_ptr<ParseData>(const Tokenizer::Token&)> TerminalDecorator;
        std::map<unsigned int, TerminalDecorator> mTerminalDecorators;

        typedef std::function<std::shared_ptr<ParseData>(typename Util::MultiStack<ParseItem>::PathView)> Reducer;
        std::map<unsigned int, Reducer> mReducers;        

    public:            
        Earley(const Grammar &grammar)
            : EarleyBase(grammar)
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

        std::vector<std::shared_ptr<ParseData>> parse(Tokenizer::Stream &stream) const
        {
            std::vector<std::shared_ptr<ParseData>> terminalData;
            std::vector<unsigned int> terminalIndices;

            auto tokenListener = [&](const Tokenizer::Token &token) {
                auto it = mTerminalDecorators.find(token.value);
                std::shared_ptr<ParseData> parseData;
                if(it != mTerminalDecorators.end()) {
                    parseData = it->second(token);
                }
                terminalData.push_back(parseData);
                terminalIndices.push_back(token.value);
            };

            std::vector<std::set<Item>> completedSets = computeSets(stream, tokenListener);
            
            Util::MultiStack<ParseItem> parseStacks;
            parseRule(completedSets, terminalIndices, grammar().startRule(), 0, (unsigned int)(completedSets.size() - 1), parseStacks, terminalData);

            std::vector<std::shared_ptr<ParseData>> results;
            for(size_t i=0; i<parseStacks.size(); i++) {
                results.push_back(parseStacks.stack(i).back().data);
            }

            return results;
        }

    private:
        void parseRule(const std::vector<std::set<Item>> &completedSets, const std::vector<unsigned int> &terminalIndices, unsigned int rule, unsigned int start, unsigned int end, Util::MultiStack<ParseItem> &parseStacks, std::vector<std::shared_ptr<ParseData>> &terminalData) const
        {
            auto stackBegin = parseStacks.stack(parseStacks.size() - 1).end();
            bool first = true;

            for(const auto &item : completedSets[end]) {
                if(item.rule != rule || item.start != start) {
                    continue;
                }

                std::vector<std::vector<unsigned int>> partitions = findPartitions(completedSets, terminalIndices, item.rule, item.rhs, start, end);
                const Grammar::RHS &rhsSymbols = grammar().rules()[item.rule].rhs[item.rhs];
        
                for(const auto &partition : partitions) {
                    size_t stack;
                    if(first) {
                        stack = parseStacks.size() - 1;
                        first = false;
                    } else {
                        stack = parseStacks.addStack(stackBegin);
                    }

                    for(unsigned int j = 0; j<partition.size(); j++) {
                        unsigned int ji = (unsigned int)(partition.size() - 1 - j);
                        unsigned int pstart = partition[ji];
                        unsigned int pend = (ji == 0) ? end : partition[ji - 1];

                        switch(rhsSymbols[j].type) {
                            case Grammar::Symbol::Type::Terminal:
                            {
                                ParseItem newItem {
                                    .type = ParseItem::Type::Terminal,
                                    .index = rhsSymbols[j].index,
                                    .data = terminalData[pstart]
                                };
                                parseStacks.stack(stack).push_back(std::move(newItem));
                                break;
                            }
                            case Grammar::Symbol::Type::Nonterminal:
                            {
                                parseRule(completedSets, terminalIndices, rhsSymbols[j].index, pstart, pend, parseStacks, terminalData);
                                while(parseStacks.size() > stack + 1) {
                                    auto stackEnd = parseStacks.stack(stack).end();
                                    parseStacks.joinStack(parseStacks.size() - 1, stackEnd);
                                }
                                break;
                            }
                            case Grammar::Symbol::Type::Epsilon:
                                break;
                        }
                    }

                    auto it = mReducers.find(rule);
                    if(it != mReducers.end()) {
                        size_t stack = parseStacks.size() - 1;
                        auto stackEnd = parseStacks.stack(stack).end();
                        auto begins = parseStacks.connect(stackBegin, stackEnd);
                        for(unsigned int i=0; i<begins.size(); i++) {
                            typename Util::MultiStack<ParseItem>::PathView view(begins[i], stackEnd);
                            std::shared_ptr<ParseData> data = it->second(view);
                            ParseItem newItem {
                                .type = ParseItem::Type::Nonterminal,
                                .index = rule,
                                .data = data
                            };

                            if(i < begins.size() - 1) {
                                size_t newStack = parseStacks.addStack(stackBegin);
                                parseStacks.stack(newStack).push_back(std::move(newItem));
                            } else {
                                parseStacks.relocateStack(stack, stackBegin);
                                parseStacks.stack(stack).push_back(std::move(newItem));
                            }
                        }
                    }
                }
            }
        }
    };
}
#endif