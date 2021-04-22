#include "Grammar.hpp"

Grammar::Grammar(std::vector<Rule> &&rules, unsigned int startRule)
: mRules(std::move(rules)), mStartRule(startRule)
{
}

const std::vector<Grammar::Rule> &Grammar::rules() const
{
    return mRules;
}

unsigned int Grammar::startRule() const
{
    return mStartRule;
}

void addSymbol(std::set<unsigned int> &set, unsigned int symbol, bool &changed)
{
    if(set.count(symbol) == 0) {
        set.insert(symbol);
        changed = true;
    }
}

void addSet(std::set<unsigned int> &set, const std::set<unsigned int> &source, bool &changed)
{
    for(unsigned int s : source) {
        addSymbol(set, s, changed);
    }
}

void addFirstSet(std::set<unsigned int> &set, const Grammar::Symbol &symbol, const std::vector<std::set<unsigned int>> &firstSets, bool &changed)
{
    switch(symbol.type) {
        case Grammar::Symbol::Type::Terminal:
            addSymbol(set, symbol.index, changed);
            break;
        
        case Grammar::Symbol::Type::Nonterminal:
            addSet(set, firstSets[symbol.index], changed);
            break;
    }
}

bool isNullable(const Grammar::Symbol &symbol, const std::set<unsigned int> &nullable) {
    switch(symbol.type) {
        case Grammar::Symbol::Type::Terminal:
            return false;
        case Grammar::Symbol::Type::Epsilon:
            return true;
        case Grammar::Symbol::Type::Nonterminal:
            return nullable.count(symbol.index) > 0;
    }
    return false;
}

void Grammar::computeSets(std::vector<std::set<unsigned int>> &firstSets, std::vector<std::set<unsigned int>> &followSets, std::set<unsigned int> &nullableNonterminals) const
{
    firstSets.resize(mRules.size());
    followSets.resize(mRules.size());

    bool changed = true;
    while(changed) {
        changed = false;
        for(unsigned int i=0; i<mRules.size(); i++) {
            for(const RHS &rhs : mRules[i].rhs) {
                bool foundNonNullable = false;
                for(unsigned int j=0; j<rhs.symbols.size(); j++) {
                    const Symbol &symbol = rhs.symbols[j];
                    if(!foundNonNullable) {
                        addFirstSet(firstSets[i], symbol, firstSets, changed);
                    }

                    if(!isNullable(symbol, nullableNonterminals)) {
                        foundNonNullable = true;
                    }

                    if(symbol.type == Symbol::Type::Nonterminal) {
                        bool foundOtherNonNullable = false;
                        for(unsigned int k = j+1; k<rhs.symbols.size(); k++) {
                            const Symbol &otherSymbol = rhs.symbols[k];
                            addFirstSet(followSets[j], otherSymbol, firstSets, changed);
                            if(!isNullable(otherSymbol, nullableNonterminals)) {
                                foundOtherNonNullable = true;
                                break;
                            }
                        }

                        if(!foundOtherNonNullable) {
                            addSet(followSets[j], followSets[i], changed);
                        }
                    }
                }

                if(!foundNonNullable) {
                    addSymbol(nullableNonterminals, i, changed);
                }
            }
        }       
    }
}
