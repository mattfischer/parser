#include "Parser/Earley.hpp"

#include <iostream>

namespace Parser
{
    Earley::Earley(const Grammar &grammar)
    : mGrammar(grammar)
    {
    }

    std::vector<std::set<Earley::Item>> Earley::computeSets(Tokenizer::Stream &stream) const
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