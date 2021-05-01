#include "LLParser.hpp"

#include <algorithm>

LLParser::LLParser(const Grammar &grammar)
: mGrammar(grammar)
{
    std::vector<std::set<unsigned int>> firstSets;
    std::vector<std::set<unsigned int>> followSets;
    std::set<unsigned int> nullableNonterminals;
    mGrammar.computeSets(firstSets, followSets, nullableNonterminals);

    mValid = computeParseTable(firstSets, followSets, nullableNonterminals);
}

bool LLParser::addParseTableEntry(unsigned int rule, unsigned int symbol, unsigned int rhs)
{
    if(mParseTable[rule*mGrammar.terminals().size()+symbol] == UINT_MAX) {
        mParseTable[rule*mGrammar.terminals().size()+symbol] = rhs;
        return true;
    } else {
        mConflict.rule = rule;
        mConflict.symbol = symbol;
        mConflict.rhs1 = mParseTable[rule*mGrammar.terminals().size()+symbol];
        mConflict.rhs2 = rhs;
        return false;
    }
}

bool LLParser::addParseTableEntries(unsigned int rule, const std::set<unsigned int> &symbols, unsigned int rhs)
{
    for(unsigned int s : symbols) {
        if(!addParseTableEntry(rule, s, rhs)) {
            return false;
        }
    }

    return true;
}

bool LLParser::computeParseTable(const std::vector<std::set<unsigned int>> &firstSets, std::vector<std::set<unsigned int>> &followSets, std::set<unsigned int> &nullableNonterminals)
{
    mParseTable.resize(mGrammar.rules().size() * mGrammar.terminals().size(), UINT_MAX);

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

bool LLParser::valid() const
{
    return mValid;
}

const LLParser::Conflict &LLParser::conflict() const
{
    return mConflict;
}

const Grammar &LLParser::grammar() const
{
    return mGrammar;
}

unsigned int LLParser::rhs(unsigned int rule, unsigned int symbol) const
{
    return mParseTable[rule*mGrammar.terminals().size() + symbol];

}