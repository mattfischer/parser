#include "Parser.hpp"

#include <algorithm>

Parser::Parser(const std::vector<Rule> &rules, unsigned int startRule)
: mRules(rules)
{
    std::vector<std::set<unsigned int>> firstSets;
    std::vector<std::set<unsigned int>> followSets;
    std::set<unsigned int> nullableNonterminals;
    computeSets(firstSets, followSets, nullableNonterminals);

    mValid = computeParseTable(firstSets, followSets, nullableNonterminals);
    mStartRule = startRule;
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

void addFirstSet(std::set<unsigned int> &set, const Parser::Symbol &symbol, const std::vector<std::set<unsigned int>> &firstSets, bool &changed)
{
    switch(symbol.type) {
        case Parser::Symbol::Type::Terminal:
            addSymbol(set, symbol.index, changed);
            break;
        
        case Parser::Symbol::Type::Nonterminal:
            addSet(set, firstSets[symbol.index], changed);
            break;
    }
}

bool isNullable(const Parser::Symbol &symbol, const std::set<unsigned int> &nullable) {
    switch(symbol.type) {
        case Parser::Symbol::Type::Terminal:
            return false;
        case Parser::Symbol::Type::Epsilon:
            return true;
        case Parser::Symbol::Type::Nonterminal:
            return nullable.count(symbol.index) > 0;
    }
    return false;
}

void Parser::computeSets(std::vector<std::set<unsigned int>> &firstSets, std::vector<std::set<unsigned int>> &followSets, std::set<unsigned int> &nullableNonterminals)
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

bool Parser::addParseTableEntry(unsigned int rule, unsigned int symbol, unsigned int rhs)
{
    if(mParseTable[rule*mNumSymbols+symbol] == UINT_MAX) {
        mParseTable[rule*mNumSymbols+symbol] = rhs;
        return true;
    } else {
        mConflict.rule = rule;
        mConflict.symbol = symbol;
        mConflict.rhs1 = mParseTable[rule*mNumSymbols+symbol];
        mConflict.rhs2 = rhs;
        return false;
    }
}

bool Parser::addParseTableEntries(unsigned int rule, const std::set<unsigned int> &symbols, unsigned int rhs)
{
    for(unsigned int s : symbols) {
        if(!addParseTableEntry(rule, s, rhs)) {
            return false;
        }
    }

    return true;
}

bool Parser::computeParseTable(const std::vector<std::set<unsigned int>> &firstSets, std::vector<std::set<unsigned int>> &followSets, std::set<unsigned int> &nullableNonterminals)
{
    unsigned int maxSymbol = 0;
    for(const auto &rule : mRules) {
        for(const auto &rhs : rule.rhs) {
            for(const auto &symbol : rhs.symbols) {
                if(symbol.type == Symbol::Type::Terminal) {
                    maxSymbol = std::max(maxSymbol, symbol.index);
                }
            }
        }
    }

    mNumSymbols = maxSymbol + 1;
    mParseTable.resize(mRules.size() * mNumSymbols);

    for(unsigned int i=0; i<mRules.size(); i++) {
        const Rule &rule = mRules[i];

        for(unsigned int j=0; j<mNumSymbols; j++) {
            mParseTable[i*mNumSymbols+j] = UINT_MAX;
        }

        for(unsigned int j=0; j<rule.rhs.size(); j++) {
            const Symbol &symbol = rule.rhs[j].symbols[0];
            switch(symbol.type) {
                case Symbol::Type::Terminal:
                    if(!addParseTableEntry(i, symbol.index, j)) {
                        return false;
                    }
                    break;
                
                case Symbol::Type::Nonterminal:
                    if(!addParseTableEntries(i, firstSets[symbol.index], j)) {
                        return false;
                    }

                    if(nullableNonterminals.count(symbol.index) > 0) {
                        if(!addParseTableEntries(i, followSets[symbol.index], j)) {
                            return false;
                        }
                    }
                    break;
                
                case Symbol::Type::Epsilon:
                    if(!addParseTableEntries(i, followSets[i], j)) {
                        return false;
                    }
                    break;
            }
        }
    }

    return true;
}

bool Parser::valid() const
{
    return mValid;
}

const Parser::Conflict &Parser::conflict() const
{
    return mConflict;
}

void Parser::parse(Tokenizer &tokenizer) const
{
    parseRule(mStartRule, tokenizer);
}

void Parser::parseRule(unsigned int rule, Tokenizer &tokenizer) const
{
    unsigned int rhs = mParseTable[rule*mNumSymbols + tokenizer.nextToken().index];

    if(rhs == UINT_MAX) {
        throw ParseException(tokenizer.nextToken().index);
    }   

    const std::vector<Symbol> &symbols = mRules[rule].rhs[rhs].symbols;
    for(unsigned int i=0; i<symbols.size(); i++) {
        const Symbol &symbol = symbols[i];
        switch(symbol.type) {
            case Symbol::Type::Terminal:
                if(tokenizer.nextToken().index == symbol.index) {
                    tokenizer.consumeToken();
                } else {
                    throw ParseException(tokenizer.nextToken().index);
                }
                break;

            case Symbol::Type::Nonterminal:
                parseRule(symbol.index, tokenizer);
                break;
        }
    }
}