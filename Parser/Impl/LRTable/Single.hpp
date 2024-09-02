#ifndef PARSER_IMPL_LRTABLE_SINGLE_HPP
#define PARSER_IMPL_LRTABLE_SINGLE_HPP

#include "Parser/Impl/LRTable/Base.hpp"

namespace Parser::Impl::LRTable {
    class Single : public Base {
    public:
        Single(const Grammar &grammar);

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

        bool valid() const;
        const Conflict &conflict() const;

        bool isAccept(unsigned int state) const;
        unsigned int nextStateForRule(unsigned int state, unsigned int rule) const;

        template<typename S, typename R, typename E> void process(unsigned int state, unsigned int symbol, S shift, R reduce, E error) const
        {
            const ParseTableEntry &entry = mParseTable.at(state, symbol);
            
            switch(entry.type) {
                case ParseTableEntry::Type::Shift:
                {
                    shift(entry.index);
                    break;
                }

                case ParseTableEntry::Type::Reduce:
                {
                    const Reduction &reduction = mReductions[entry.index];
                    reduce(reduction.rule, reduction.rhs);
                    break;
                }

                case ParseTableEntry::Type::Error:
                {
                    error();
                    break;
                }
            }
        }

    protected:
        bool computeParseTable(const std::vector<State> &states);

        struct ParseTableEntry {
            enum class Type {
                Shift,
                Reduce,
                Error
            };
            Type type;
            unsigned int index;
        };

        struct Reduction {
            bool operator==(const Reduction &other) {
                return rule == other.rule && rhs == other.rhs;
            }

            unsigned int rule;
            unsigned int rhs;
        };

        Util::Table<ParseTableEntry> mParseTable;
        std::vector<Reduction> mReductions;
        std::set<unsigned int> mAcceptStates;

        bool mValid;
        Conflict mConflict;
    };
}
#endif