#include "Parser.hpp"

Parser::Parser(const std::vector<Rule> &rules)
: mRules(rules)
{
    computeFirstSets();
}

void Parser::computeFirstSets()
{
    mFirstSets.resize(mRules.size());

    bool changed = true;
    while(changed) {
        changed = false;
        for(unsigned int i=0; i<mRules.size(); i++) {
            for(const RHS &rhs : mRules[i].rhs) {
                const Symbol &symbol = rhs.symbols[0];
                switch(symbol.type) {
                    case Symbol::Type::Terminal:
                        if(mFirstSets[i].count(symbol.index) == 0) {
                            mFirstSets[i].insert(symbol.index);
                            changed = true;
                        }
                        break;
                    
                    case Symbol::Type::Nonterminal:
                        for(unsigned int s : mFirstSets[symbol.index]) {
                            if(mFirstSets[i].count(s) == 0) {
                                mFirstSets[i].insert(s);
                                changed = true;
                            }   
                        }
                        break;
                }
            }
        }
    }
}

void Parser::parse(Tokenizer &tokenizer) const
{
    parseRule(0, tokenizer);
}

void Parser::parseRule(unsigned int rule, Tokenizer &tokenizer) const
{
    unsigned int rhs;
    for(unsigned int i=0; i<mRules[rule].rhs.size(); i++) {
        unsigned int nextToken = tokenizer.nextToken().index;
        const Symbol &symbol = mRules[rule].rhs[i].symbols[0];
        bool match = false;
        switch(symbol.type) {
            case Symbol::Type::Terminal:
                if(symbol.index == nextToken) {
                    match = true;
                }
                break;
            
            case Symbol::Type::Nonterminal:
                if(mFirstSets[symbol.index].count(nextToken) > 0) {
                    match = true;
                }
                break;
        }

        if(match) {
            rhs = i;
            break;
        }
    }

    const std::vector<Symbol> &symbols = mRules[rule].rhs[rhs].symbols;
    for(unsigned int i=0; i<symbols.size(); i++) {
        const Symbol &symbol = symbols[i];
        switch(symbol.type) {
            case Symbol::Type::Terminal:
                if(tokenizer.nextToken().index == symbol.index) {
                    tokenizer.consumeToken();
                } else {
                }
                break;

            case Symbol::Type::Nonterminal:
                parseRule(symbol.index, tokenizer);
                break;
        }
    }
}