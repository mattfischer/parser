#ifndef PARSER_EARLEY_HPP
#define PARSER_EARLEY_HPP

#include "Parser/Grammar.hpp"
#include "Util/MultiStack.hpp"
#include "Tokenizer.hpp"

#include <set>
#include <vector>
#include <algorithm>
#include <functional>

namespace Parser
{
    class Earley
    {
    public:
        Earley(const Grammar &grammar);

        struct Item {
            unsigned int rule;
            unsigned int rhs;
            unsigned int pos;
            unsigned int start;

            bool operator<(const Item &other) const;
        };

        template<typename ParseData> class ParseSession
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

                typedef ParseItem* iterator;
            };

            typedef std::function<std::shared_ptr<ParseData>(const Tokenizer::Token&)> TerminalDecorator;
            typedef std::function<std::shared_ptr<ParseData>(typename Util::MultiStack<ParseItem>::iterator, typename Util::MultiStack<ParseItem>::Locator)> Reducer;
            
            ParseSession(const Earley &parser);

            void addTerminalDecorator(const std::string &terminal, TerminalDecorator terminalDecorator);
            void addReducer(const std::string &rule, Reducer reducer);

            std::vector<std::shared_ptr<ParseData>> parse(Tokenizer::Stream &stream) const;
        
        private:
            void parseRule(const std::vector<std::set<Earley::Item>> &completedSets, const std::vector<unsigned int> &terminalIndices, unsigned int rule, unsigned int start, unsigned int end, Util::MultiStack<ParseItem> &parseStacks, std::vector<std::shared_ptr<ParseData>> &terminalData) const;

            const Earley &mParser;
            std::map<unsigned int, TerminalDecorator> mTerminalDecorators;
            std::map<unsigned int, Reducer> mReducers;        
        };

    private:
        const Grammar &mGrammar;

        std::vector<Earley::Item> predict(unsigned int ruleIndex, unsigned int pos) const;
        std::vector<Earley::Item> scan(std::set<Item> &items, const Grammar::Symbol &symbol) const;    
        void populateSets(std::vector<Item> &items, std::vector<std::set<Item>> &active, std::vector<std::set<Item>> &completed, unsigned int pos) const;
        std::vector<unsigned int> findStarts(const std::vector<std::set<Earley::Item>> &completedSets, const std::vector<unsigned int> &terminalIndices, const Grammar::Symbol &symbol, unsigned int end, unsigned int minStart) const;
        std::vector<std::vector<unsigned int>> findPartitions(const std::vector<std::set<Earley::Item>> &completedSets, const std::vector<unsigned int> &terminalIndices, unsigned int rule, unsigned int rhs, unsigned int start, unsigned int end) const;
 

        void printSets(const std::vector<std::set<Item>> &active, const std::vector<std::set<Item>> &completed) const;
        void printSets(const std::vector<std::set<Item>> &completed) const;
        void printItem(const Item &item) const;

        typedef std::function<void(const Tokenizer::Token&)> TokenListener;
        std::vector<std::set<Item>> computeSets(Tokenizer::Stream &stream, TokenListener tokenListener) const;
    };

    template<typename ParseData> Earley::ParseSession<ParseData>::ParseSession(const Earley &parser) : mParser(parser) {}

    template<typename ParseData> void Earley::ParseSession<ParseData>::addTerminalDecorator(const std::string &terminal, TerminalDecorator terminalDecorator)
    {
        unsigned int terminalIndex = mParser.mGrammar.terminalIndex(terminal);
        if(terminalIndex != UINT_MAX) {
            mTerminalDecorators[terminalIndex] = terminalDecorator;
        }
    }

    template<typename ParseData> void Earley::ParseSession<ParseData>::addReducer(const std::string &rule, Reducer reducer)
    {
        unsigned int ruleIndex = mParser.mGrammar.ruleIndex(rule);
        if(ruleIndex != UINT_MAX) {
            mReducers[ruleIndex] = reducer;
        }
    }

    template<typename ParseData> std::vector<std::shared_ptr<ParseData>> Earley::ParseSession<ParseData>::parse(Tokenizer::Stream &stream) const
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

        std::vector<std::set<Earley::Item>> completedSets = mParser.computeSets(stream, tokenListener);
        
        Util::MultiStack<ParseItem> parseStacks;
        parseRule(completedSets, terminalIndices, mParser.mGrammar.startRule(), 0, (unsigned int)(completedSets.size() - 1), parseStacks, terminalData);

        std::vector<std::shared_ptr<ParseData>> results;
        for(size_t i=0; i<parseStacks.size(); i++) {
            results.push_back(parseStacks.back(i).data);
        }

        return results;
    }

    template<typename ParseData> void Earley::ParseSession<ParseData>::parseRule(const std::vector<std::set<Earley::Item>> &completedSets, const std::vector<unsigned int> &terminalIndices, unsigned int rule, unsigned int start, unsigned int end, Util::MultiStack<ParseItem> &parseStacks, std::vector<std::shared_ptr<ParseData>> &terminalData) const
    {
        Util::MultiStack<ParseItem>::Locator stackBegin = parseStacks.end(parseStacks.size() - 1);
        bool first = true;

        for(const auto &item : completedSets[end]) {
            if(item.rule != rule || item.start != start) {
                continue;
            }

            std::vector<std::vector<unsigned int>> partitions = mParser.findPartitions(completedSets, terminalIndices, item.rule, item.rhs, start, end);
            const Grammar::RHS &rhsSymbols = mParser.mGrammar.rules()[item.rule].rhs[item.rhs];
    
            for(const auto &partition : partitions) {
                size_t stack;
                if(first) {
                    stack = parseStacks.size() - 1;
                    first = false;
                } else {
                    stack = parseStacks.add(stackBegin);
                }

                for(unsigned int j = 0; j<partition.size(); j++) {
                    unsigned int ji = (unsigned int)(partition.size() - 1 - j);
                    unsigned int pstart = partition[ji];
                    unsigned int pend = (ji == 0) ? end : partition[ji - 1];

                    switch(rhsSymbols[j].type) {
                        case Grammar::Symbol::Type::Terminal:
                        {
                            ParseItem newItem;
                            newItem.type = ParseItem::Type::Terminal;
                            newItem.index = rhsSymbols[j].index;
                            newItem.data = terminalData[pstart];
                            parseStacks.push_back(stack, std::move(newItem));
                            break;
                        }
                        case Grammar::Symbol::Type::Nonterminal:
                        {
                            parseRule(completedSets, terminalIndices, rhsSymbols[j].index, pstart, pend, parseStacks, terminalData);
                            while(parseStacks.size() > stack + 1) {
                                parseStacks.join(parseStacks.size() - 1, parseStacks.end(stack));
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
                    MultiStack<ParseItem>::Locator end = parseStacks.end(stack);
                    std::vector<MultiStack<ParseItem>::iterator> begins = parseStacks.connect(stackBegin, end);
                    for(unsigned int i=0; i<begins.size(); i++) {
                        std::shared_ptr<ParseData> data = it->second(begins[i], end);
                        ParseItem newItem;
                        newItem.type = ParseItem::Type::Nonterminal;
                        newItem.index = rule;
                        newItem.data = data;
                        if(i < begins.size() - 1) {
                            size_t newStack = parseStacks.add(stackBegin);
                            parseStacks.push_back(newStack, std::move(newItem));
                        } else {
                            parseStacks.relocate(stack, stackBegin);
                            parseStacks.push_back(stack, std::move(newItem));
                        }
                    }
                }
            }
        }
    }
}
#endif