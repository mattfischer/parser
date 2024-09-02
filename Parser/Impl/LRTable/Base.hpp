#ifndef PARSER_IMPL_LRTABLE_BASE_HPP
#define PARSER_IMPL_LRTABLE_BASE_HPP

#include "Parser/Grammar.hpp"
#include "Util/Table.hpp"

#include <set>
#include <map>
#include <vector>

namespace Parser::Impl::LRTable {
    class Base {
    public:
        Base(const Grammar &grammar);

    protected:
        struct Item {
            bool operator<(const Item &other) const;
            bool operator==(const Item &other) const;

            unsigned int rule;
            unsigned int rhs;
            unsigned int pos;
        };

        struct State {
            std::set<Item> items;
            std::map<unsigned int, unsigned int> transitions;
        };

        const Grammar &grammar() const;

        void computeClosure(std::set<Item> &items) const;
        std::vector<State> computeStates() const;

        virtual const std::set<unsigned int> &getReduceLookahead(unsigned int, unsigned int) const = 0;

        void printStates(const std::vector<State> &states) const;
    
        unsigned int symbolIndex(const Grammar::Symbol &symbol) const;
        unsigned int terminalIndex(unsigned int terminal) const;
        unsigned int ruleIndex(unsigned int rule) const;

    private:
        const Grammar &mGrammar;
    };
}
#endif