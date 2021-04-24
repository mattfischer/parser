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

    struct ParseItem {
        struct Data {};
        enum class Type {
            Terminal,
            Nonterminal
        };
        Type type;
        unsigned int index;
        std::unique_ptr<Data> data;
    };

    typedef std::function<void(ParseItem&, const Tokenizer::Token&)> TerminalDecorator;
    typedef std::function<std::unique_ptr<ParseItem::Data>(std::vector<ParseItem>&, unsigned int, unsigned int, unsigned int)> Reducer;
    void parse(Tokenizer::Stream &stream, TerminalDecorator terminalDecorator, Reducer reducer) const;

private:
    bool addParseTableEntry(unsigned int rule, unsigned int symbol, unsigned int rhs);
    bool addParseTableEntries(unsigned int rule, const std::set<unsigned int> &symbols, unsigned int rhs);
    bool computeParseTable(const std::vector<std::set<unsigned int>> &firstSets, std::vector<std::set<unsigned int>> &followSets, std::set<unsigned int> &nullableNonterminals);

    void parseRule(unsigned int rule, Tokenizer::Stream &stream, std::vector<ParseItem> &parseStack, TerminalDecorator terminalDecorator, Reducer reducer) const;
    
    const Grammar &mGrammar;
    std::vector<unsigned int> mParseTable;  
    unsigned int mNumSymbols;
    bool mValid;
    Conflict mConflict;
};

#endif