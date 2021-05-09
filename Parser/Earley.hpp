#ifndef PARSER_EARLEY_HPP
#define PARSER_EARLEY_HPP

#include "Parser/Grammar.hpp"
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

        template<typename Data> struct ParseItem {
            enum class Type {
                Terminal,
                Nonterminal,
                Multistack
            };
            Type type;
            unsigned int index;
            std::vector<std::shared_ptr<Data>> data;
            std::vector<std::vector<ParseItem<Data>>> stacks;
        };

        template<typename ParseData> class ParseSession
        {
        public:
            typedef std::function<std::shared_ptr<ParseData>(const Tokenizer::Token&)> TerminalDecorator;
            typedef std::function<std::shared_ptr<ParseData>(ParseItem<ParseData>*, unsigned int)> Reducer;
            
            ParseSession(const Earley &parser) : mParser(parser) {}

            void addTerminalDecorator(const std::string &terminal, TerminalDecorator terminalDecorator)
            {
                unsigned int terminalIndex = mParser.mGrammar.terminalIndex(terminal);
                if(terminalIndex != UINT_MAX) {
                    mTerminalDecorators[terminalIndex] = terminalDecorator;
                }
            }

            void addReducer(const std::string &rule, Reducer reducer)
            {
                unsigned int ruleIndex = mParser.mGrammar.ruleIndex(rule);
                if(ruleIndex != UINT_MAX) {
                    mReducers[ruleIndex] = reducer;
                }
            }

            std::shared_ptr<ParseData> parse(Tokenizer::Stream &stream) const
            {
                std::vector<std::shared_ptr<ParseData>> terminalData;

                auto tokenListener = [&](const Tokenizer::Token &token) {
                    auto it = mTerminalDecorators.find(token.value);
                    std::shared_ptr<ParseData> parseData;
                    if(it != mTerminalDecorators.end()) {
                        parseData = it->second(token);
                    }
                    terminalData.push_back(parseData);
                };

                std::vector<std::set<Earley::Item>> completedSets = mParser.computeSets(stream, tokenListener);

                std::vector<ParseItem<ParseData>> parseStack;
                parseRule(completedSets, mParser.mGrammar.startRule(), 0, (unsigned int)(completedSets.size() - 1), parseStack, terminalData);
                return parseStack[0].data[0];
            }
        
            void parseRule(const std::vector<std::set<Earley::Item>> &completedSets, unsigned int rule, unsigned int start, unsigned int end, std::vector<ParseItem<ParseData>> &parseStack, std::vector<std::shared_ptr<ParseData>> &terminalData) const
            {
                unsigned int numReductions = 0;
                ParseItem<ParseData> newItem;
                Reducer reducer;    
                auto it = mReducers.find(rule);
                if(it == mReducers.end()) {
                    newItem.type = ParseItem<ParseData>::Type::Multistack;
                } else {
                    reducer = it->second;
                    newItem.type = ParseItem<ParseData>::Type::Nonterminal;
                    newItem.index = rule;    
                }
                
                unsigned int parseStackStart = (unsigned int)parseStack.size();

                for(const auto &item : completedSets[end]) {
                    if(item.rule != rule || item.start != start) {
                        continue;
                    }

                    std::vector<std::vector<unsigned int>> partitions = mParser.findPartitions(completedSets, item.rule, item.rhs, start, end);
                    const Grammar::RHS &rhsSymbols = mParser.mGrammar.rules()[item.rule].rhs[item.rhs];
            
                    for(const auto &partition : partitions) {
                        if(!reducer && numReductions == 1) {
                            std::vector<ParseItem<ParseData>> stack;
                            stack.insert(stack.end(), parseStack.begin() + parseStackStart, parseStack.end());
                            newItem.stacks.push_back(std::move(stack));
                            parseStack.erase(parseStack.begin() + parseStackStart, parseStack.end());
                        }

                        for(unsigned int j = 0; j<partition.size(); j++) {
                            unsigned int ji = (unsigned int)(partition.size() - 1 - j);
                            unsigned int pstart = partition[ji];
                            unsigned int pend = (ji == 0) ? end : partition[ji - 1];

                            switch(rhsSymbols[j].type) {
                                case Grammar::Symbol::Type::Terminal:
                                {
                                    ParseItem<ParseData> newItem;
                                    newItem.type = ParseItem<ParseData>::Type::Terminal;
                                    newItem.index = rhsSymbols[j].index;
                                    newItem.data.push_back(terminalData[pstart]);
                                    parseStack.push_back(std::move(newItem));
                                    break;
                                }
                                case Grammar::Symbol::Type::Nonterminal:
                                {
                                    parseRule(completedSets, rhsSymbols[j].index, pstart, pend, parseStack, terminalData);
                                    break;
                                }
                                case Grammar::Symbol::Type::Epsilon:
                                    break;
                            }
                        }

                        if(reducer) {
                            std::vector<std::shared_ptr<ParseData>> newData = reduce(parseStack, parseStackStart, reducer);
                            newItem.data.insert(newItem.data.end(), newData.begin(), newData.end());
                            parseStack.erase(parseStack.begin() + parseStackStart, parseStack.end());
                        } else if(numReductions > 0) {
                            std::vector<ParseItem<ParseData>> stack;
                            stack.insert(stack.end(), parseStack.begin() + parseStackStart, parseStack.end());
                            newItem.stacks.push_back(std::move(stack));
                            parseStack.erase(parseStack.begin() + parseStackStart, parseStack.end());
                        }

                        numReductions++;
                    }
                }

                if(reducer || numReductions > 1) {
                    parseStack.push_back(std::move(newItem));
                }
            }

            std::vector<std::shared_ptr<ParseData>> reduce(std::vector<ParseItem<ParseData>> &parseStack, unsigned int parseStackStart, Reducer &reducer) const
            {
                std::vector<std::shared_ptr<ParseData>> results;

                bool foundMultistack = false;
                for(unsigned int i=parseStackStart; i<parseStack.size(); i++) {
                    if(parseStack[i].type == ParseItem<ParseData>::Type::Multistack) {
                        ParseItem<ParseData> item = std::move(parseStack[i]);
                        parseStack.erase(parseStack.begin() + i);
                        for(const auto &stack : item.stacks) {
                            parseStack.insert(parseStack.begin() + i, stack.begin(), stack.end());
                            std::vector<std::shared_ptr<ParseData>> newResults = reduce(parseStack, parseStackStart, reducer);
                            results.insert(results.end(), newResults.begin(), newResults.end());
                            parseStack.erase(parseStack.begin() + i, parseStack.begin() + i + stack.size());
                        }
                        parseStack.insert(parseStack.begin() + i, std::move(item));
                        foundMultistack = true;
                        break;
                    }
                }

                if(!foundMultistack) {
                    std::shared_ptr<ParseData> parseData = reducer(&parseStack[parseStackStart], (unsigned int)(parseStack.size() - parseStackStart));
                    results.push_back(parseData);
                }

                return results;
            }

        private:
            const Earley &mParser;
            std::map<unsigned int, TerminalDecorator> mTerminalDecorators;
            std::map<unsigned int, Reducer> mReducers;        
        };

    private:
        const Grammar &mGrammar;

        std::vector<Earley::Item> predict(unsigned int ruleIndex, unsigned int pos) const;
        std::vector<Earley::Item> scan(std::set<Item> &items, const Grammar::Symbol &symbol) const;    
        void populateSets(std::vector<Item> &items, std::vector<std::set<Item>> &active, std::vector<std::set<Item>> &completed, unsigned int pos) const;
        std::vector<unsigned int> findStarts(const std::vector<std::set<Earley::Item>> &completedSets, const Grammar::Symbol &symbol, unsigned int end, unsigned int minStart) const;
        std::vector<std::vector<unsigned int>> findPartitions(const std::vector<std::set<Earley::Item>> &completedSets, unsigned int rule, unsigned int rhs, unsigned int start, unsigned int end) const;
 

        void printSets(const std::vector<std::set<Item>> &active, const std::vector<std::set<Item>> &completed) const;
        void printSets(const std::vector<std::set<Item>> &completed) const;
        void printItem(const Item &item) const;

        typedef std::function<void(const Tokenizer::Token&)> TokenListener;
        std::vector<std::set<Item>> computeSets(Tokenizer::Stream &stream, TokenListener tokenListener) const;

    };
}
#endif