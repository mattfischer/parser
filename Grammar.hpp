#ifndef GRAMMAR_HPP
#define GRAMMAR_HPP

#include <string>
#include <vector>
#include <set>

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
        std::string name;
    };

    typedef std::vector<Symbol> RHS;

    struct Rule {
        std::string lhs;
        std::vector<RHS> rhs;
    };

    Grammar(std::vector<Rule> &&rules, unsigned int startRule);

    const std::vector<Rule> &rules() const;
    unsigned int startRule() const;

    unsigned int ruleIndex(const std::string &name) const;

    void computeSets(std::vector<std::set<unsigned int>> &firstSets, std::vector<std::set<unsigned int>> &followSets, std::set<unsigned int> &nullableNonterminals) const;

private:
    std::vector<Rule> mRules;
    unsigned int mStartRule;
};

#endif