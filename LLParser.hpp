#ifndef LLPARSER_HPP
#define LLPARSER_HPP

#include "Grammar.hpp"
#include "Tokenizer.hpp"

#include <vector>
#include <set>

class LLParser {
public:
    class ParseException : public std::exception {
    public:
        ParseException(unsigned int s) { symbol = s; }

        unsigned int symbol;
    };

    LLParser(const Grammar &grammar);

    bool valid() const;

    struct Conflict {
        unsigned int rule;
        unsigned int symbol;
        unsigned int rhs1;
        unsigned int rhs2;
    };
    const Conflict &conflict() const;

    template<typename Data> struct ParseItem {
        enum class Type {
            Terminal,
            Nonterminal
        };
        Type type;
        unsigned int index;
        std::unique_ptr<Data> data;
    };

    template<typename ParseData> using TerminalDecorator = std::function<std::unique_ptr<ParseData>(const Tokenizer::Token&)>;
    template<typename ParseData> using Reducer = std::function<std::unique_ptr<ParseData>(std::vector<ParseItem<ParseData>>&, unsigned int, unsigned int, unsigned int)>;

    template<typename ParseData> void parse(Tokenizer::Stream &stream, TerminalDecorator<ParseData> terminalDecorator, Reducer<ParseData> reducer) const
    {
        std::vector<ParseItem<ParseData>> parseStack;
        parseRule(mGrammar.startRule(), stream, parseStack, terminalDecorator, reducer);
    }

private:
    bool addParseTableEntry(unsigned int rule, unsigned int symbol, unsigned int rhs);
    bool addParseTableEntries(unsigned int rule, const std::set<unsigned int> &symbols, unsigned int rhs);
    bool computeParseTable(const std::vector<std::set<unsigned int>> &firstSets, std::vector<std::set<unsigned int>> &followSets, std::set<unsigned int> &nullableNonterminals);

    template<typename ParseData> void parseRule(unsigned int rule, Tokenizer::Stream &stream, std::vector<ParseItem<ParseData>> &parseStack, TerminalDecorator<ParseData> terminalDecorator, Reducer<ParseData> reducer) const
    {
        unsigned int rhs = mParseTable[rule*mNumSymbols + stream.nextToken().index];

        if(rhs == UINT_MAX) {
            throw ParseException(stream.nextToken().index);
        }   

        unsigned int parseStackStart = (unsigned int)parseStack.size();
        const std::vector<Grammar::Symbol> &symbols = mGrammar.rules()[rule].rhs[rhs];
        for(unsigned int i=0; i<symbols.size(); i++) {
            const Grammar::Symbol &symbol = symbols[i];
            switch(symbol.type) {
                case Grammar::Symbol::Type::Terminal:
                    if(stream.nextToken().index == symbol.index) {
                        ParseItem<ParseData> parseItem;
                        parseItem.type = ParseItem<ParseData>::Type::Terminal;
                        parseItem.index = symbol.index;
                        parseItem.data = terminalDecorator(stream.nextToken());
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

        std::unique_ptr<ParseData> data = reducer(parseStack, parseStackStart, rule, rhs);
        if(data) {
            parseStack.erase(parseStack.begin() + parseStackStart, parseStack.end());

            ParseItem<ParseData> parseItem;
            parseItem.type = ParseItem<ParseData>::Type::Nonterminal;
            parseItem.index = rule;
            parseItem.data = std::move(data);
            parseStack.push_back(std::move(parseItem));
        }
    }
    
    const Grammar &mGrammar;
    std::vector<unsigned int> mParseTable;  
    unsigned int mNumSymbols;
    bool mValid;
    Conflict mConflict;
};

#endif