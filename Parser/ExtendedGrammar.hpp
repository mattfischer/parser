#ifndef PARSER_EXTENDEDGRAMMAR_HPP
#define PARSER_EXTENDEDGRAMMAR_HPP

#include <vector>
#include <string>
#include <memory>

#include "Parser/Grammar.hpp"

namespace Parser {

    class ExtendedGrammar
    {
    public:
        struct RhsNode {
            enum class Type {
                Symbol,
                Sequence,
                OneOf,
                OneOrMore,
                ZeroOrMore,
                ZeroOrOne
            };
            RhsNode(Type t) : type(t) {}

            Type type;
        };

        struct RhsNodeSymbol : public RhsNode {
            enum class SymbolType {
                Terminal,
                Nonterminal
            };

            RhsNodeSymbol(SymbolType t, unsigned int i) : RhsNode(Type::Symbol), symbolType(t), index(i) {}

            SymbolType symbolType;
            unsigned int index;
        };

        struct RhsNodeChild : public RhsNode {
            RhsNodeChild(Type t, std::unique_ptr<RhsNode> &&c) : RhsNode(t), child(std::move(c)) {}

            std::unique_ptr<RhsNode> child;
        };

        struct RhsNodeChildren : public RhsNode {
            template<typename...Children> RhsNodeChildren(Type t, Children&&... c) : RhsNode(t)
            {
                children.reserve(sizeof...(c));
                (children.push_back(std::move(c)), ...);
            }

            std::vector<std::unique_ptr<RhsNode>> children;
        };

        struct Rule {
            std::string lhs;
            std::unique_ptr<RhsNode> rhs;
        };

        ExtendedGrammar(std::vector<std::string> &&terminals, std::vector<Rule> &&rules, unsigned int startRule);

        std::unique_ptr<Grammar> makeGrammar() const;

    private:
        void populateRule(std::vector<Grammar::Rule> &grammarRules, unsigned int index, const RhsNode &rhsNode) const;
        void populateRhs(Grammar::RHS &grammarRhs, const RhsNode &rhsNode, std::vector<Grammar::Rule> &grammarRules, const std::string &ruleName) const;
        void populateSymbol(Grammar::Symbol &grammarSymbol, const RhsNode &rhsNode, std::vector<Grammar::Rule> &grammarRules, const std::string &ruleName) const;

        std::vector<std::string> mTerminals;
        std::vector<Rule> mRules;
        unsigned int mStartRule;
    };
}

#endif