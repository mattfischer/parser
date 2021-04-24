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
    unsigned int maxSymbol = 0;
    for(const auto &rule : mGrammar.rules()) {
        for(const auto &rhs : rule.rhs) {
            for(const auto &symbol : rhs) {
                if(symbol.type == Grammar::Symbol::Type::Terminal) {
                    maxSymbol = std::max(maxSymbol, symbol.index);
                }
            }
        }
    }

    mNumSymbols = maxSymbol + 1;
    mParseTable.resize(mGrammar.rules().size() * mNumSymbols);

    for(unsigned int i=0; i<mGrammar.rules().size(); i++) {
        const Grammar::Rule &rule = mGrammar.rules()[i];

        for(unsigned int j=0; j<mNumSymbols; j++) {
            mParseTable[i*mNumSymbols+j] = UINT_MAX;
        }

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

void LLParser::parse(Tokenizer::Stream &stream, TerminalDecorator terminalDecorator, Reducer reducer) const
{
    std::vector<ParseItem> parseStack;
    parseRule(mGrammar.startRule(), stream, parseStack, terminalDecorator, reducer);
}

void LLParser::parseRule(unsigned int rule, Tokenizer::Stream &stream, std::vector<ParseItem> &parseStack, TerminalDecorator terminalDecorator, Reducer reducer) const
{
    unsigned int rhs = mParseTable[rule*mNumSymbols + stream.nextToken().index];

    if(rhs == UINT_MAX) {
        throw ParseException(stream.nextToken().index);
    }   

    unsigned int parseStackStart = parseStack.size();
    const std::vector<Grammar::Symbol> &symbols = mGrammar.rules()[rule].rhs[rhs];
    for(unsigned int i=0; i<symbols.size(); i++) {
        const Grammar::Symbol &symbol = symbols[i];
        switch(symbol.type) {
            case Grammar::Symbol::Type::Terminal:
                if(stream.nextToken().index == symbol.index) {
                    ParseItem parseItem;
                    parseItem.type = ParseItem::Type::Terminal;
                    parseItem.index = symbol.index;
                    terminalDecorator(parseItem, stream.nextToken());
                    parseStack.push_back(std::move(parseItem));
                    stream.consumeToken();
                } else {
                    throw ParseException(stream.nextToken().index);
                }
                break;

            case Grammar::Symbol::Type::Nonterminal:
                parseRule(symbol.index, stream, parseStack, terminalDecorator, reducer);
                break;
        }
    }

    std::unique_ptr<ParseItem::Data> data = reducer(parseStack, parseStackStart, rule, rhs);
    if(data) {
        parseStack.erase(parseStack.begin() + parseStackStart, parseStack.end());

        ParseItem parseItem;
        parseItem.type = ParseItem::Type::Nonterminal;
        parseItem.index = rule;
        parseItem.data = std::move(data);
        parseStack.push_back(std::move(parseItem));
    }
}