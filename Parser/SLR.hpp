#ifndef PARSER_SLR_HPP
#define PARSER_SLR_HPP

#include "Parser/Grammar.hpp"
#include "Util/Table.hpp"

#include "Tokenizer.hpp"

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

        class Session
        {
        public:
            Session(const SLR &parser)
            : mParser(parser)
            {
            }
        
            void parse(Tokenizer::Stream &stream)
            {
                unsigned int state = 0;
                std::vector<unsigned int> stateStack;

                bool done = false;
                while(!done) {
                    stateStack.push_back(state);
                    const ParseTableEntry &entry = mParser.mParseTable.at(state, mParser.terminalIndex(stream.nextToken().value));
                    switch(entry.type) {
                        case ParseTableEntry::Type::Shift:
                            stream.consumeToken();
                            state = entry.index;
                            break;
                        
                        case ParseTableEntry::Type::Reduce:
                        {
                            const Reduction &reduction = mParser.mReductions[entry.index];
                            const Grammar::RHS &rhs = mParser.mGrammar.rules()[reduction.rule].rhs[reduction.rhs];
                            for(unsigned int i=0; i<rhs.size(); i++) {
                                if(rhs[i].type != Grammar::Symbol::Type::Epsilon) {
                                    stateStack.pop_back();
                                }
                            }

                            if(reduction.rule == mParser.mGrammar.startRule()) {
                                done = true;
                            } else {
                                state = stateStack.back();
                                const ParseTableEntry &newEntry = mParser.mParseTable.at(state, mParser.ruleIndex(reduction.rule));
                                state = newEntry.index;
                            }
                            break;
                        }
                    }
                }
            }

        private:
            const SLR &mParser;
        };

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
        unsigned int terminalIndex(unsigned int terminal) const;
        unsigned int ruleIndex(unsigned int rule) const;

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

        struct Reduction {
            bool operator==(const Reduction &other) {
                return rule == other.rule && rhs == other.rhs;
            }

            unsigned int rule;
            unsigned int rhs;
        };

        const Grammar &mGrammar;
        Util::Table<ParseTableEntry> mParseTable;
        std::vector<Reduction> mReductions;
        bool mValid;
        Conflict mConflict;
    };
}
#endif