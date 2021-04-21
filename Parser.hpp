#ifndef PARSER_HPP
#define PARSER_HPP

#include "Tokenizer.hpp"

#include <vector>
#include <set>

class Parser {
public:
    struct Symbol {
        enum class Type {
            Terminal,
            Nonterminal,
            Epsilon
        };
        Type type;
        unsigned int index;
        std::string name;
    };

    struct RHS {
        std::vector<Symbol> symbols;
    };

    struct Rule {
        std::vector<RHS> rhs;
    };

    class ParseException : public std::exception {
    public:
        ParseException(unsigned int s) { symbol = s; }

        unsigned int symbol;
    };

    Parser(const std::vector<Rule> &rules, unsigned int startRule);

    bool valid() const;

    struct Conflict {
        unsigned int rule;
        unsigned int symbol;
        unsigned int rhs1;
        unsigned int rhs2;
    };
    const Conflict &conflict() const;

    void parse(Tokenizer::Stream &stream) const;

private:
    void computeSets(std::vector<std::set<unsigned int>> &firstSets, std::vector<std::set<unsigned int>> &followSets, std::set<unsigned int> &nullableNonterminals);
    bool addParseTableEntry(unsigned int rule, unsigned int symbol, unsigned int rhs);
    bool addParseTableEntries(unsigned int rule, const std::set<unsigned int> &symbols, unsigned int rhs);
    bool computeParseTable(const std::vector<std::set<unsigned int>> &firstSets, std::vector<std::set<unsigned int>> &followSets, std::set<unsigned int> &nullableNonterminals);

    void parseRule(unsigned int rule, Tokenizer::Stream &stream) const;
    
    unsigned int mStartRule;
    const std::vector<Rule> &mRules;
    std::vector<unsigned int> mParseTable;  
    unsigned int mNumSymbols;
    bool mValid;
    Conflict mConflict;
};

#endif