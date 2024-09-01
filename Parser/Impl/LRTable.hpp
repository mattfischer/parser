#ifndef PARSER_IMPL_LR_TABLE_HPP
#define PARSER_IMPL_LR_TABLE_HPP

#include "Parser/Grammar.hpp"
#include "Util/Table.hpp"

#include <set>
#include <map>
#include <vector>
#include <functional>

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

        typedef std::function<std::set<unsigned int>(unsigned int, unsigned int)> GetReduceLookahead;

        void printStates(const std::vector<State> &states, GetReduceLookahead getReduceLookahead) const;
    
        unsigned int symbolIndex(const Grammar::Symbol &symbol) const;
        unsigned int terminalIndex(unsigned int terminal) const;
        unsigned int ruleIndex(unsigned int rule) const;

    private:
        const Grammar &mGrammar;
    };
    
    class Multi : public Base {
    public:
        Multi(const Grammar &grammar);

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
                    reduce(reduction.rule, reduction.rhs, true);
                    break;
                }

                case ParseTableEntry::Type::Multi:
                {
                    const auto &entries = mMultiEntries[entry.index];
                    for(size_t j=0; j<entries.size(); j++) {
                        const auto &entry = entries[j];
                        switch(entry.type) {
                            case ParseTableEntry::Type::Shift:
                            {
                                shift(entry.index);
                                break;
                            }

                            case ParseTableEntry::Type::Reduce:
                            {
                                const Reduction &reduction = mReductions[entry.index];
                                reduce(reduction.rule, reduction.rhs, j == entries.size() - 1);
                                break;
                            }

                            default:
                                break;
                        }
                    }
                    break;
                }

                case ParseTableEntry::Type::Error:
                {
                    error();
                    break;
                }
            }
        }

    private:
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