#ifndef PARSER_GRAMMAR_HPP
#define PARSER_GRAMMAR_HPP

#include <string>
#include <vector>
#include <set>

namespace Parser {

    class Grammar {
    public:
        struct Symbol {
            enum class Type {
                Terminal,
                Nonterminal,
                Epsilon
            };
            Type type;
            unsigned int index;

            bool operator==(const Symbol &other) const {
                return type == other.type && index == other.index;
            }
        };

        typedef std::vector<Symbol> RHS;

        struct Rule {
            std::string lhs;
            std::vector<RHS> rhs;
        };

        Grammar(std::vector<std::string> terminals, std::vector<Rule> rules, unsigned int startRule);

        const std::vector<Rule> &rules() const;
        unsigned int startRule() const;
        const std::vector<std::string> &terminals() const;

        unsigned int terminalIndex(const std::string &name) const;
        unsigned int ruleIndex(const std::string &name) const;

        void computeSets(std::vector<std::set<unsigned int>> &firstSets, std::vector<std::set<unsigned int>> &followSets, std::set<unsigned int> &nullableNonterminals) const;

        void print() const;

    private:
        std::vector<std::string> mTerminals;
        std::vector<Rule> mRules;
        unsigned int mStartRule;
    };
}

#endif