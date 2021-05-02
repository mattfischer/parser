#include "LL1.hpp"

#include <algorithm>

namespace Parser {

    LL1::LL1(const Grammar &grammar)
    : mGrammar(grammar)
    {
        std::vector<std::set<unsigned int>> firstSets;
        std::vector<std::set<unsigned int>> followSets;
        std::set<unsigned int> nullableNonterminals;
        mGrammar.computeSets(firstSets, followSets, nullableNonterminals);

        mValid = computeParseTable(firstSets, followSets, nullableNonterminals);
    }

    bool LL1::addParseTableEntry(unsigned int rule, unsigned int symbol, unsigned int rhs)
    {
        if(mParseTable.at(rule, symbol) == UINT_MAX) {
            mParseTable.at(rule, symbol) = rhs;
            return true;
        } else {
            mConflict.rule = rule;
            mConflict.symbol = symbol;
            mConflict.rhs1 = mParseTable.at(rule, symbol);
            mConflict.rhs2 = rhs;
            return false;
        }
    }

    bool LL1::addParseTableEntries(unsigned int rule, const std::set<unsigned int> &symbols, unsigned int rhs)
    {
        for(unsigned int s : symbols) {
            if(!addParseTableEntry(rule, s, rhs)) {
                return false;
            }
        }

        return true;
    }

    bool LL1::computeParseTable(const std::vector<std::set<unsigned int>> &firstSets, std::vector<std::set<unsigned int>> &followSets, std::set<unsigned int> &nullableNonterminals)
    {
        mParseTable.resize(mGrammar.rules().size(), mGrammar.terminals().size(), UINT_MAX);

        for(unsigned int i=0; i<mGrammar.rules().size(); i++) {
            const Grammar::Rule &rule = mGrammar.rules()[i];

            for(unsigned int j=0; j<rule.rhs.size(); j++) {
                const Grammar::Symbol &symbol = rule.rhs[j][0];
                switch(symbol.type) {
                    case Grammar::Symbol::Type::Terminal:
                        if(!addParseTableEntry(i, symbol.index, j)) {
                            return false;
                        }
                        break;
                    
                    case Grammar::Symbol::Type::Nonterminal:
                        if(!addParseTableEntries(i, firstSets[symbol.index], j)) {
                            return false;
                        }

                        if(nullableNonterminals.count(symbol.index) > 0) {
                            if(!addParseTableEntries(i, followSets[symbol.index], j)) {
                                return false;
                            }
                        }
                        break;
                    
                    case Grammar::Symbol::Type::Epsilon:
                        if(!addParseTableEntries(i, followSets[i], j)) {
                            return false;
                        }
                        break;
                }
            }
        }

        return true;
    }

    bool LL1::valid() const
    {
        return mValid;
    }

    const LL1::Conflict &LL1::conflict() const
    {
        return mConflict;
    }

    const Grammar &LL1::grammar() const
    {
        return mGrammar;
    }

    unsigned int LL1::rhs(unsigned int rule, unsigned int symbol) const
    {
        return mParseTable.at(rule, symbol);

    }
}