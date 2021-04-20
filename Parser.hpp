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
            Nonterminal
        };
        Type type;
        unsigned int index;
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

    void parse(Tokenizer &tokenizer) const;

private:
    std::vector<std::set<unsigned int>> computeFirstSets();
    bool computeParseTable(const std::vector<std::set<unsigned int>> &firstSets);

    void parseRule(unsigned int rule, Tokenizer &tokenizer) const;
    
    unsigned int mStartRule;
    const std::vector<Rule> &mRules;
    std::vector<unsigned int> mParseTable;  
    unsigned int mNumSymbols;
    bool mValid;
    Conflict mConflict;
};

#endif