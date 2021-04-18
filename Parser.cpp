#include "Parser.hpp"

#include <algorithm>

Parser::Parser(const std::vector<Rule> &rules)
: mRules(rules)
{
    std::vector<std::set<unsigned int>> firstSets = computeFirstSets();
    mValid = computeParseTable(firstSets);
}

std::vector<std::set<unsigned int>> Parser::computeFirstSets()
{
    std::vector<std::set<unsigned int>> firstSets;
    firstSets.resize(mRules.size());

    bool changed = true;
    while(changed) {
        changed = false;
        for(unsigned int i=0; i<mRules.size(); i++) {
            for(const RHS &rhs : mRules[i].rhs) {
                const Symbol &symbol = rhs.symbols[0];
                switch(symbol.type) {
                    case Symbol::Type::Terminal:
                        if(firstSets[i].count(symbol.index) == 0) {
                            firstSets[i].insert(symbol.index);
                            changed = true;
                        }
                        break;
                    
                    case Symbol::Type::Nonterminal:
                        for(unsigned int s : firstSets[symbol.index]) {
                            if(firstSets[i].count(s) == 0) {
                                firstSets[i].insert(s);
                                changed = true;
                            }   
                        }
                        break;
                }
            }
        }
    }

    return firstSets;
}

bool Parser::computeParseTable(const std::vector<std::set<unsigned int>> &firstSets)
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
            unsigned int rhs = (unsigned int)(rule.rhs.size());
            for(unsigned int k=0; k<rule.rhs.size(); k++) {
                const Symbol &symbol = rule.rhs[k].symbols[0];
                bool match = false;
                switch(symbol.type) {
                    case Symbol::Type::Terminal:
                        match = (symbol.index == j);
                        break;
                    
                    case Symbol::Type::Nonterminal:
                        match = (firstSets[symbol.index].count(j) > 0);
                        break;
                }

                if(match) {
                    if(rhs == rule.rhs.size()) {
                        rhs = k;
                    } else {
                        mConflict.rule = i;
                        mConflict.rhs1 = rhs;
                        mConflict.rhs2 = k;
                        return false;
                    }
                }
            }

            mParseTable[i*mNumSymbols+j] = rhs;
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
    parseRule(0, tokenizer);
}

void Parser::parseRule(unsigned int rule, Tokenizer &tokenizer) const
{
    unsigned int rhs = mParseTable[rule*mNumSymbols + tokenizer.nextToken().index];

    if(rhs > mRules[rule].rhs.size()) {
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