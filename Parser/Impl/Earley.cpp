#include "Parser/Impl/Earley.hpp"

#include <iostream>

namespace Parser
{
    namespace Impl
    {
        Earley::Earley(const Grammar &grammar)
        : Base(grammar)
        {
        }

        std::vector<std::set<Earley::Item>> Earley::computeSets(Tokenizer::Stream &stream, TokenListener tokenListener) const
        {
            std::vector<std::set<Item>> completed;
            std::vector<std::set<Item>> active;

            active.push_back(std::set<Item>());
            completed.push_back(std::set<Item>());
            
            std::vector<Item> items;
            unsigned int pos = 0;
            const Grammar::Rule &startRule = mGrammar.rules()[mGrammar.startRule()];
            for(unsigned int i=0; i<startRule.rhs.size(); i++) {
                items.push_back(Item{mGrammar.startRule(), i, 0, 0});
            }
            populateSets(items, active, completed, pos);

            while(true) {
                std::vector<Item> newItems = scan(active[pos], Grammar::Symbol{Grammar::Symbol::Type::Terminal, stream.nextToken().value});
                active.push_back(std::set<Item>());
                completed.push_back(std::set<Item>());

                pos++;
                populateSets(newItems, active, completed, pos);
            
                tokenListener(stream.nextToken());
                if(stream.nextToken().value == stream.tokenizer().endValue()) {
                    break;
                } else {    
                    stream.consumeToken();
                }
            }

            return completed;
        }

        std::vector<Earley::Item> Earley::predict(unsigned int ruleIndex, unsigned int pos) const
        {
            std::vector<Item> items;

            const Grammar::Rule &rule = mGrammar.rules()[ruleIndex];
            for(unsigned int i=0; i<rule.rhs.size(); i++) {
                for(unsigned int j=0; j<=rule.rhs[i].size(); j++) {
                    Item item = {ruleIndex, i, j, pos};
                    items.push_back(item);
                    if(j < rule.rhs[i].size() && rule.rhs[i][j].type != Grammar::Symbol::Type::Epsilon) {
                        break;
                    }
                }
            }

            return items;
        }

        std::vector<Earley::Item> Earley::scan(std::set<Item> &items, const Grammar::Symbol &symbol) const
        {
            std::vector<Item> newItems;
            for(const auto &item : items) {
                if(mGrammar.rules()[item.rule].rhs[item.rhs][item.pos] == symbol) {
                    newItems.push_back(Item{item.rule, item.rhs, item.pos + 1, item.start});
                }
            }

            return newItems;
        }

        void Earley::populateSets(std::vector<Item> &items, std::vector<std::set<Item>> &active, std::vector<std::set<Item>> &completed, unsigned int pos) const
        {
            while(items.size() > 0) {
                Item item = items.back();
                items.pop_back();

                const Grammar::RHS &rhs = mGrammar.rules()[item.rule].rhs[item.rhs];
                std::vector<Item> newItems;
                if(item.pos == rhs.size()) {
                    if(completed[pos].find(item) != completed[pos].end()) {
                        continue;
                    }

                    completed[pos].insert(item);
                    newItems = scan(active[item.start], Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, item.rule});
                } else {
                    if(active[pos].find(item) != active[pos].end()) {
                        continue;
                    }
                    active[pos].insert(item);

                    const Grammar::Symbol &symbol = mGrammar.rules()[item.rule].rhs[item.rhs][item.pos];
                    if(symbol.type == Grammar::Symbol::Type::Nonterminal) {
                        newItems = predict(symbol.index, pos);
                        items.insert(items.end(), newItems.begin(), newItems.end());
                    }
                }
                items.insert(items.end(), newItems.begin(), newItems.end());
            }
        }

        std::vector<unsigned int> Earley::findStarts(const std::vector<std::set<Earley::Item>> &completedSets, const std::vector<unsigned int> &terminalIndices, const Grammar::Symbol &symbol, unsigned int end, unsigned int minStart) const
        {
            std::vector<unsigned int> starts;

            switch(symbol.type) {
                case Grammar::Symbol::Type::Terminal:
                    if(end > 0 && terminalIndices[end - 1] == symbol.index) {
                        starts.push_back(end - 1);
                    }
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

        std::vector<std::vector<unsigned int>> Earley::findPartitions(const std::vector<std::set<Earley::Item>> &completedSets, const std::vector<unsigned int> &terminalIndices, unsigned int rule, unsigned int rhs, unsigned int start, unsigned int end) const
        {
            const Grammar::RHS &rhsSymbols = mGrammar.rules()[rule].rhs[rhs];
            std::vector<std::vector<unsigned int>> partitions;

            for(unsigned int i = 0; i<rhsSymbols.size(); i++) {
                unsigned int ri = (unsigned int)(rhsSymbols.size() - 1 - i);
                const Grammar::Symbol &symbol = rhsSymbols[ri];
                if(i == 0) {
                    std::vector<unsigned int> starts = findStarts(completedSets, terminalIndices, symbol, end, start);
                    for(unsigned int s : starts) {
                        partitions.push_back(std::vector<unsigned int>{s});
                    }
                } else {
                    std::vector<std::vector<unsigned int>> oldPartitions = std::move(partitions);
                    for(unsigned int j=0; j<oldPartitions.size(); j++) {
                        unsigned int currentEnd = oldPartitions[j].back();
                        std::vector<unsigned int> starts = findStarts(completedSets, terminalIndices, symbol, currentEnd, start);
                        if(starts.size() == 1) {
                            partitions.push_back(std::move(oldPartitions[j]));
                            partitions.back().push_back(starts[0]);
                        } else {
                            for(unsigned int s : starts) {
                                partitions.push_back(oldPartitions[j]);
                                partitions.back().push_back(s);
                            }
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

        bool Earley::Item::operator<(const Item &other) const
        {
            if(rule < other.rule) return true;
            if(rule > other.rule) return false;
            if(rhs < other.rhs) return true;
            if(rhs > other.rhs) return false;
            if(pos < other.pos) return true;
            if(pos > other.pos) return false;
            if(start < other.start) return true;
            return false;
        }

        void Earley::printItem(const Item &item) const
        {
            std::cout << "<" << mGrammar.rules()[item.rule].lhs << ">: ";
            const Grammar::RHS &rhs = mGrammar.rules()[item.rule].rhs[item.rhs];
            for(unsigned int i=0; i<=rhs.size(); i++) {
                if(i == item.pos) {
                    std::cout << ". ";
                }
                if(i == rhs.size()) {
                    break;
                }
                switch(rhs[i].type) {
                    case Grammar::Symbol::Type::Nonterminal:
                        std::cout << "<" << mGrammar.rules()[rhs[i].index].lhs << ">";
                        break;
                    case Grammar::Symbol::Type::Terminal:
                        std::cout << mGrammar.terminals()[rhs[i].index];
                        break;
                    case Grammar::Symbol::Type::Epsilon:
                        std::cout << "0";
                        break;
                }
                std::cout << " ";
            }
            std::cout << "@" << item.start;
        }

        void Earley::printSets(const std::vector<std::set<Item>> &active, const std::vector<std::set<Item>> &completed) const
        {
            for(unsigned int i=0; i<active.size(); i++) {
                std::cout << "Set " << i << ":" << std::endl;
                std::cout << "  Active:" << std::endl;
                for(const auto &item : active[i]) {
                    std::cout << "    ";
                    printItem(item);
                    std::cout << std::endl;
                }
                std::cout << "  Completed:" << std::endl;
                for(const auto &item : completed[i]) {
                    std::cout << "    ";
                    printItem(item);
                    std::cout << std::endl;
                }
                std::cout << std::endl;
            }
        }

        void Earley::printSets(const std::vector<std::set<Item>> &completed) const
        {
            for(unsigned int i=0; i<completed.size(); i++) {
                std::cout << "Set " << i << ":" << std::endl;
                for(const auto &item : completed[i]) {
                    std::cout << "    ";
                    printItem(item);
                    std::cout << std::endl;
                }
                std::cout << std::endl;
            }
        }
    }
}