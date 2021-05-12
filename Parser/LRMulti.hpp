#ifndef PARSER_LR_MULTI_HPP
#define PARSER_LR_MULTI_HPP

#include "Parser/LR.hpp"

namespace Parser
{
    class LRMulti : public LR
    {
    public:
        LRMulti(const Grammar &grammar);

    protected:
        struct ParseTableEntry {
            enum class Type {
                Shift,
                Reduce,
                Multi,
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

        void addParseTableEntry(unsigned int state, unsigned int symbol, const ParseTableEntry &entry);
        void computeParseTable(const std::vector<State> &states, GetReduceLookahead getReduceLookahead);

        Util::Table<ParseTableEntry> mParseTable;
        std::vector<std::vector<ParseTableEntry>> mMultiEntries;
        std::vector<Reduction> mReductions;
        std::set<unsigned int> mAcceptStates;
    };
}
#endif