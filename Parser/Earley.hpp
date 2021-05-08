#ifndef PARSER_EARLEY_HPP
#define PARSER_EARLEY_HPP

#include "Parser/Grammar.hpp"
#include "Tokenizer.hpp"

#include <set>
#include <vector>

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
                std::vector<std::vector<unsigned int>> partitions;
                for(unsigned int i = rhsSymbols.size() - 1; i >= 0; i--) {
                    const Grammar::Symbol &symbol = rhsSymbols[i];
                    unsigned int ri = rhsSymbols.size() - 1 - i;
                    if(ri == 0) {
                        switch(symbol.type) {
                            case Grammar::Symbol::Type::Terminal:
                                partitions.push_back(std::vector<unsigned int>{end - 1});
                                break;
                            case Grammar::Symbol::Type::Epsilon:
                                partitions.push_back(std::vector<unsigned int>{end});
                                break;
                            case Grammar::Symbol::Type::Nonterminal:
                                for(const auto &item : completedSets[end]) {
                                    if(item.rule == symbol.index && item.start >= start) {
                                        partitions.push_back(std::vector<unsigned int>{item.start});
                                    }
                                }
                                break;
                        }
                    } else {
                        bool repeat = false;
                        for(unsigned int j=0; j<partitions.size() || repeat; j++) {
                            if(repeat) {
                                j--;
                                repeat = false;
                            }

                            if(partitions[j].size() == ri + 1) {
                                break;
                            }
                        
                            unsigned int currentEnd = partitions[j].back();
                            switch(symbol.type) {
                                case Grammar::Symbol::Type::Terminal:
                                    partitions[j].push_back(currentEnd - 1);
                                    break;
                                case Grammar::Symbol::Type::Epsilon:
                                    partitions[j].push_back(currentEnd);
                                    break;
                                case Grammar::Symbol::Type::Nonterminal:
                                {
                                    bool first = true;
                                    bool found = false;
                                    for(const auto &item : completedSets[currentEnd]) {
                                        if(item.rule == symbol.index && item.start >= start) {
                                            found = true;
                                            if(first) {
                                                partitions[j].push_back(item.start);
                                                first = false;
                                            } else {
                                                partitions.push_back(partitions[j]);
                                                partitions.back().push_back(item.start);
                                            }
                                        }
                                    }
                                    if(!found) {
                                        partitions.erase(partitions.begin() + j);
                                        repeat = true;
                                    }
                                    break;
                                }
                            }
                        } 
                    }

                    if(partitions.size() == 0) {
                        return std::unique_ptr<ParseData>();
                    }

                    if(i == 0) {
                        break;
                    }
                }

                for(const auto &partition : partitions) {
                    if(partition[partition.size() - 1] != start) {
                        continue;
                    }

                    for(unsigned int j = 0; j<partition.size(); j++) {
                        unsigned int ji = partition.size() - 1 - j;
                        unsigned int pstart = partition[ji];
                        unsigned int pend;
                        if(j == partition.size() - 1) {
                            pend = end;
                        } else {
                            pend = partition[ji - 1];
                        }
                        if(rhsSymbols[j].type == Grammar::Symbol::Type::Nonterminal) {
                            parseRule(completedSets, rhsSymbols[j].index, pstart, pend);
                        }
                    }
                }
                return std::unique_ptr<ParseData>();
            }

            std::unique_ptr<ParseData> parseRule2(const std::vector<std::set<Earley::Item>> &completedSets, unsigned int rule, unsigned int end, unsigned int &start, bool &found) const
            {
                found = false;
                for(const auto &item : completedSets[end]) {
                    if(item.rule != rule) {
                        continue;
                    }

                    start = item.start;
                    const Grammar::RHS &rhs = mParser.mGrammar.rules()[item.rule].rhs[item.rhs];
                    unsigned int i = (unsigned int)(rhs.size() - 1);
                    unsigned int p = end;
                    while(true) {
                        bool valid = true;
                        switch(rhs[i].type) {
                            case Grammar::Symbol::Type::Terminal:
                                p--;
                                break;
                            case Grammar::Symbol::Type::Nonterminal:
                            {
                                unsigned int s;                   
                                bool f;
                                std::unique_ptr<ParseData> d = parseRule(completedSets, rhs[i].index, p, s, f);
                                if(!f) {
                                    valid = false;
                                    break;
                                }
                                p = s - 1;
                                break;
                            }
                            case Grammar::Symbol::Type::Epsilon:
                                break;
                        }
                        
                        if(!valid || i == 0) {
                            break;
                        } else {
                            i--;
                        }
                    }

                    if(p == start) {
                        found = true;
                        break;
                    }
                }
                return std::unique_ptr<ParseData>();
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