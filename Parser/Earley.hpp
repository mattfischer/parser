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
            std::shared_ptr<Data> data;
            std::vector<std::vector<ParseItem<Data>>> stacks;

            typedef ParseItem* iterator;
        };

        template<typename ParseData> class ParseSession
        {
        public:
            typedef std::function<std::shared_ptr<ParseData>(const Tokenizer::Token&)> TerminalDecorator;
            typedef std::function<std::shared_ptr<ParseData>(typename ParseItem<ParseData>::iterator, typename ParseItem<ParseData>::iterator)> Reducer;
            
            ParseSession(const Earley &parser);

            void addTerminalDecorator(const std::string &terminal, TerminalDecorator terminalDecorator);
            void addReducer(const std::string &rule, Reducer reducer);

            std::vector<std::shared_ptr<ParseData>> parse(Tokenizer::Stream &stream) const;
        
        private:
            void parseRule(const std::vector<std::set<Earley::Item>> &completedSets, const std::vector<unsigned int> &terminalIndices, unsigned int rule, unsigned int start, unsigned int end, std::vector<ParseItem<ParseData>> &parseStack, std::vector<std::shared_ptr<ParseData>> &terminalData) const;
            std::vector<std::shared_ptr<ParseData>> reduce(std::vector<ParseItem<ParseData>> &parseStack, unsigned int parseStackStart, const Reducer &reducer) const;

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
        
        std::vector<ParseItem<ParseData>> parseStack;
        parseRule(completedSets, terminalIndices, mParser.mGrammar.startRule(), 0, (unsigned int)(completedSets.size() - 1), parseStack, terminalData);

        std::vector<std::shared_ptr<ParseData>> results;
        if(parseStack.size() > 0) {
            if(parseStack[0].type == ParseItem<ParseData>::Type::Multistack) {
                for(auto &stack : parseStack[0].stacks) {
                    results.push_back(stack[0].data);
                }
            } else {
                results.push_back(parseStack[0].data);
            }
        }

        return results;
    }

    template<typename ParseData> void Earley::ParseSession<ParseData>::parseRule(const std::vector<std::set<Earley::Item>> &completedSets, const std::vector<unsigned int> &terminalIndices, unsigned int rule, unsigned int start, unsigned int end, std::vector<ParseItem<ParseData>> &parseStack, std::vector<std::shared_ptr<ParseData>> &terminalData) const
    {
        unsigned int numReductions = 0;
        ParseItem<ParseData> multiStackItem;
        multiStackItem.type = ParseItem<ParseData>::Type::Multistack;

        unsigned int parseStackStart = (unsigned int)parseStack.size();

        for(const auto &item : completedSets[end]) {
            if(item.rule != rule || item.start != start) {
                continue;
            }

            std::vector<std::vector<unsigned int>> partitions = mParser.findPartitions(completedSets, terminalIndices, item.rule, item.rhs, start, end);
            const Grammar::RHS &rhsSymbols = mParser.mGrammar.rules()[item.rule].rhs[item.rhs];
    
            for(const auto &partition : partitions) {
                if(numReductions == 1) {
                    std::vector<ParseItem<ParseData>> stack;
                    stack.insert(stack.end(), parseStack.begin() + parseStackStart, parseStack.end());
                    multiStackItem.stacks.push_back(std::move(stack));
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
                            newItem.data = terminalData[pstart];
                            parseStack.push_back(std::move(newItem));
                            break;
                        }
                        case Grammar::Symbol::Type::Nonterminal:
                        {
                            parseRule(completedSets, terminalIndices, rhsSymbols[j].index, pstart, pend, parseStack, terminalData);
                            break;
                        }
                        case Grammar::Symbol::Type::Epsilon:
                            break;
                    }
                }

                auto it = mReducers.find(rule);
                if(it != mReducers.end()) {
                    std::vector<std::shared_ptr<ParseData>> newData = reduce(parseStack, parseStackStart, it->second);
                    parseStack.erase(parseStack.begin() + parseStackStart, parseStack.end());
                    for(auto &data : newData) {
                        ParseItem<ParseData> newItem;
                        newItem.type = ParseItem<ParseData>::Type::Nonterminal;
                        newItem.index = rule;
                        newItem.data = data;
                        if(newData.size() == 1 && numReductions == 0) {
                            parseStack.push_back(newItem);
                        } else {
                            multiStackItem.stacks.push_back(std::vector<ParseItem<ParseData>>{std::move(newItem)});
                        }
                    }
                    numReductions += (unsigned int)newData.size();
                } else if(numReductions > 0) {
                    std::vector<ParseItem<ParseData>> stack;
                    stack.insert(stack.end(), parseStack.begin() + parseStackStart, parseStack.end());
                    multiStackItem.stacks.push_back(std::move(stack));
                    parseStack.erase(parseStack.begin() + parseStackStart, parseStack.end());
                    numReductions++;
                }
            }
        }

        if(numReductions > 1) {
            parseStack.push_back(std::move(multiStackItem));
        }
    }

    template<typename ParseData> std::vector<std::shared_ptr<ParseData>> Earley::ParseSession<ParseData>::reduce(std::vector<ParseItem<ParseData>> &parseStack, unsigned int parseStackStart, const Reducer &reducer) const
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
            std::shared_ptr<ParseData> parseData = reducer(&parseStack[parseStackStart], &parseStack[0] + parseStack.size());
            results.push_back(parseData);
        }

        return results;
    }

}
#endif