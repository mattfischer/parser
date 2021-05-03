#ifndef PARSER_SLR_HPP
#define PARSER_SLR_HPP

#include "Parser/Grammar.hpp"
#include "Util/Table.hpp"

#include <set>
#include <map>
#include <vector>

namespace Parser {

    class SLR
    {
    public:
        SLR(const Grammar &grammar);

        bool valid() const;
    
        const Grammar &grammar() const;

        struct Conflict {
            enum class Type {
                ShiftReduce,
                ReduceReduce
            };
            Type type;
            unsigned int symbol;
            unsigned int item1;
            unsigned int item2;
        };

        const Conflict &conflict() const;

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

        unsigned int symbolIndex(const Grammar::Symbol &symbol) const;
        void computeClosure(std::set<Item> &items) const;
        std::vector<State> computeStates() const;
        bool computeParseTable(const std::vector<State> &states);

        void printStates(const std::vector<State> &states) const;

        struct ParseTableEntry {
            enum class Type {
                Shift,
                Reduce,
                Error
            };
            Type type;
            unsigned int index;
        };

        const Grammar &mGrammar;
        Util::Table<ParseTableEntry> mParseTable;
        bool mValid;
        Conflict mConflict;
    };
}
#endif