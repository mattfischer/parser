#ifndef PARSER_EARLEY_HPP
#define PARSER_EARLEY_HPP

#include "Parser/Grammar.hpp"
#include "Tokenizer.hpp"

#include <set>
#include <vector>
#include <algorithm>

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

        template<typename ParseData> class Session
        {
        public:
            Session(const Earley &parser) : mParser(parser) {}

            std::unique_ptr<ParseData> parse(Tokenizer::Stream &stream) const
            {
                std::vector<std::set<Earley::Item>> completedSets = mParser.computeSets(stream);
                mParser.printSets(completedSets);

                std::unique_ptr<ParseData> tree = parseRule(completedSets, mParser.mGrammar.startRule(), 0, (unsigned int)(completedSets.size() - 1));
                return tree;
            }
        
            std::unique_ptr<ParseData> parseRule(const std::vector<std::set<Earley::Item>> &completedSets, unsigned int rule, unsigned int start, unsigned int end) const
            {
                for(const auto &item : completedSets[end]) {
                    if(item.rule == rule && item.start == start) {
                        std::unique_ptr<ParseData> result = parseRhs(completedSets, item.rule, item.rhs, start, end);
                        if(result) {
                            return result;
                        }
                    }
                }

                return std::unique_ptr<ParseData>();
            }

            std::unique_ptr<ParseData> parseRhs(const std::vector<std::set<Earley::Item>> &completedSets, unsigned int rule, unsigned int rhs, unsigned int start, unsigned int end) const
            {
                const Grammar::RHS &rhsSymbols = mParser.mGrammar.rules()[rule].rhs[rhs];
                std::vector<std::vector<unsigned int>> partitions = findPartitions(completedSets, rule, rhs, start, end);

                for(const auto &partition : partitions) {
                    for(unsigned int j = 0; j<partition.size(); j++) {
                        unsigned int ji = (unsigned int)(partition.size() - 1 - j);
                        unsigned int pstart = partition[ji];
                        unsigned int pend = (j == partition.size() - 1) ? end : partition[ji - 1];

                        if(rhsSymbols[j].type == Grammar::Symbol::Type::Nonterminal) {
                            parseRule(completedSets, rhsSymbols[j].index, pstart, pend);
                        }
                    }
                }
                return std::unique_ptr<ParseData>();
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
        };

    private:
        const Grammar &mGrammar;

        std::vector<Earley::Item> predict(unsigned int ruleIndex, unsigned int pos) const;
        std::vector<Earley::Item> scan(std::set<Item> &items, const Grammar::Symbol &symbol) const;    
        void populateSets(std::vector<Item> &items, std::vector<std::set<Item>> &active, std::vector<std::set<Item>> &completed, unsigned int pos) const;
        void printSets(const std::vector<std::set<Item>> &active, const std::vector<std::set<Item>> &completed) const;
        void printSets(const std::vector<std::set<Item>> &completed) const;
        void printItem(const Item &item) const;
        std::vector<std::set<Item>> computeSets(Tokenizer::Stream &stream) const;

    };
}
#endif