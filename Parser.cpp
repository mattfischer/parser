#include "Parser.hpp"

#include <algorithm>

Parser::Parser(const std::vector<Rule> &rules, unsigned int startRule)
: mRules(rules)
{
    std::vector<std::set<unsigned int>> firstSets = computeFirstSets();
    mValid = computeParseTable(firstSets);
    mStartRule = startRule;
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
            mParseTable[i*mNumSymbols+j] = (unsigned int)rule.rhs.size();
        }

        for(unsigned int j=0; j<rule.rhs.size(); j++) {
            const Symbol &symbol = rule.rhs[j].symbols[0];
            switch(symbol.type) {
                case Symbol::Type::Terminal:
                    if(mParseTable[i*mNumSymbols+symbol.index] == rule.rhs.size()) {
                        mParseTable[i*mNumSymbols+symbol.index] = j;
                    } else {
                        mConflict.rule = i;
                        mConflict.symbol = symbol.index;
                        mConflict.rhs1 = mParseTable[i*mNumSymbols+symbol.index];
                        mConflict.rhs2 = j;
                        return false;
                    } 
                    break;
                
                case Symbol::Type::Nonterminal:
                    for(unsigned int s : firstSets[symbol.index]) {
                        if(mParseTable[i*mNumSymbols+s] == rule.rhs.size()) {
                            mParseTable[i*mNumSymbols+s] = j;
                        } else {
                            mConflict.rule = i;
                            mConflict.symbol = s;
                            mConflict.rhs1 = mParseTable[i*mNumSymbols+s];
                            mConflict.rhs2 = j;
                            return false;
                        }   
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