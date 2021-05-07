#ifndef PARSER_EARLEY_HPP
#define PARSER_EARLEY_HPP

#include "Parser/Grammar.hpp"
#include "Tokenizer.hpp"

#include <set>

namespace Parser
{
    class Earley
    {
    public:
        Earley(const Grammar &grammar);
    
        void parse(Tokenizer::Stream &stream) const;

    private:
        const Grammar &mGrammar;

        struct Item {
            unsigned int rule;
            unsigned int rhs;
            unsigned int pos;
            unsigned int start;

            bool operator<(const Item &other) const;
        };

        std::vector<Earley::Item> predict(unsigned int ruleIndex, unsigned int pos) const;
        std::vector<Earley::Item> scan(std::set<Item> &items, const Grammar::Symbol &symbol) const;    
        void populateSets(std::vector<Item> &items, std::vector<std::set<Item>> &active, std::vector<std::set<Item>> &completed, unsigned int pos) const;
        void printSets(const std::vector<std::set<Item>> &active, const std::vector<std::set<Item>> &completed) const;
        void printItem(const Item &item) const;
    
    };
}
#endif