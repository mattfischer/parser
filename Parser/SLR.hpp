#ifndef PARSER_SLR_HPP
#define PARSER_SLR_HPP

#include "LR.hpp"

#include <set>
#include <map>
#include <vector>

namespace Parser {

    class SLR : public LR
    {
    public:
        SLR(const Grammar &grammar);

    private:
        struct Item {
            bool operator<(const Item &other) const {
                if(rule < other.rule) return true;
                if(rule > other.rule) return false;
                if(rhs < other.rhs) return true;
                if(rhs > other.rhs) return false;
                if(pos < other.pos) return true;
                return false;
            }

            bool operator==(const Item &other) const {
                return rule == other.rule && rhs == other.rhs && pos == other.pos;
            }

            unsigned int rule;
            unsigned int rhs;
            unsigned int pos;
        };

        struct State {
            std::set<Item> items;
            std::map<unsigned int, unsigned int> transitions;
        };

        void computeClosure(std::set<Item> &items) const;
        std::vector<State> computeStates() const;
        virtual bool computeParseTable(const std::vector<State> &states);

        void printStates(const std::vector<State> &states) const;
    };
}
#endif