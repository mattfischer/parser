#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <vector>

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
};

#endif