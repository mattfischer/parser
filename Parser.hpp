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

    Parser(const std::vector<Rule> &rules);

    void parse(Tokenizer &tokenizer) const;

private:
    void computeFirstSets();

    void parseRule(unsigned int rule, Tokenizer &tokenizer) const;

    const std::vector<Rule> &mRules;

    std::vector<std::set<unsigned int>> mFirstSets;    
};

#endif