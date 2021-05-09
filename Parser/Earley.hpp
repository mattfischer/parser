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
                return parseStack[0].data;
            }
        
            void parseRule(const std::vector<std::set<Earley::Item>> &completedSets, unsigned int rule, unsigned int start, unsigned int end, std::vector<ParseItem<ParseData>> &parseStack, std::vector<std::shared_ptr<ParseData>> &terminalData) const
            {
                for(const auto &item : completedSets[end]) {
                    if(item.rule != rule || item.start != start) {
                        continue;
                    }

                    std::vector<std::vector<unsigned int>> partitions = findPartitions(completedSets, item.rule, item.rhs, start, end);
                    const Grammar::RHS &rhsSymbols = mParser.mGrammar.rules()[item.rule].rhs[item.rhs];
            
                    for(const auto &partition : partitions) {
                        unsigned int parseStackStart = (unsigned int)parseStack.size();
                        for(unsigned int j = 0; j<partition.size(); j++) {
                            unsigned int ji = (unsigned int)(partition.size() - 1 - j);
                            unsigned int pstart = partition[ji];
                            unsigned int pend = (j == partition.size() - 1) ? end : partition[ji - 1];

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
                                    parseRule(completedSets, rhsSymbols[j].index, pstart, pend, parseStack, terminalData);
                                    break;
                                }
                                case Grammar::Symbol::Type::Epsilon:
                                    break;
                            }
                        }

                        auto it = mReducers.find(rule);
                        if(it != mReducers.end()) {
                            std::shared_ptr<ParseData> parseData = it->second(&parseStack[parseStackStart], (unsigned int)(parseStack.size() - parseStackStart));
                            parseStack.erase(parseStack.begin() + parseStackStart, parseStack.end());
                            ParseItem<ParseData> newItem;
                            newItem.type = ParseItem<ParseData>::Type::Nonterminal;
                            newItem.index = rule;
                            newItem.data = parseData;
                            parseStack.push_back(std::move(newItem));
                        }
                    }
                }
            }

            std::vector<unsigned int> findStarts(const std::vector<std::set<Earley::Item>> &completedSets, const Grammar::Symbol &symbol, unsigned int end, unsigned int minStart) const
            {
                std::vector<unsigned int> starts;

                switch(symbol.type) {
                    case Grammar::Symbol::Type::Terminal:
                        starts.push_back(end - 1);
                        break;
                    case Grammar::Symbol::Type::Epsilon:
                        starts.push_back(end);
                        break;
                    case Grammar::Symbol::Type::Nonterminal:
                        for(const auto &item : completedSets[end]) {
                            if(item.rule == symbol.index && item.start >= minStart) {
                                starts.push_back(item.start);
                            }
                        }
                        break;
                }

                return starts;               
            }

            std::vector<std::vector<unsigned int>> findPartitions(const std::vector<std::set<Earley::Item>> &completedSets, unsigned int rule, unsigned int rhs, unsigned int start, unsigned int end) const
            {
                const Grammar::RHS &rhsSymbols = mParser.mGrammar.rules()[rule].rhs[rhs];
                std::vector<std::vector<unsigned int>> partitions;

                for(unsigned int i = 0; i<rhsSymbols.size(); i++) {
                    unsigned int ri = (unsigned int)(rhsSymbols.size() - 1 - i);
                    const Grammar::Symbol &symbol = rhsSymbols[ri];
                    if(i == 0) {
                        std::vector<unsigned int> starts = findStarts(completedSets, symbol, end, start);
                        for(unsigned int s : starts) {
                            partitions.push_back(std::vector<unsigned int>{s});
                        }
                    } else {
                        std::vector<std::vector<unsigned int>> oldPartitions = std::move(partitions);
                        bool repeat = false;
                        for(unsigned int j=0; j<oldPartitions.size(); j++) {
                            unsigned int currentEnd = oldPartitions[j].back();
                            std::vector<unsigned int> starts = findStarts(completedSets, symbol, currentEnd, start);
                            for(unsigned int s : starts) {
                                partitions.push_back(oldPartitions[j]);
                                partitions.back().push_back(s);
                            }
                        } 
                    }

                    if(partitions.size() == 0) {
                        break;
                    }
                }

                partitions.erase(std::remove_if(partitions.begin(), partitions.end(), [&](const std::vector<unsigned int> &partition) {
                    return partition.back() != start;
                }), partitions.end());

                return partitions;
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
        void printSets(const std::vector<std::set<Item>> &active, const std::vector<std::set<Item>> &completed) const;
        void printSets(const std::vector<std::set<Item>> &completed) const;
        void printItem(const Item &item) const;

        typedef std::function<void(const Tokenizer::Token&)> TokenListener;
        std::vector<std::set<Item>> computeSets(Tokenizer::Stream &stream, TokenListener tokenListener) const;

    };
}
#endif