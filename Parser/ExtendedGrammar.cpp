#include "ExtendedGrammar.hpp"

#include <sstream>
#include <iostream>

namespace Parser {

    ExtendedGrammar::ExtendedGrammar(std::vector<std::string> terminals, std::vector<Rule> rules, unsigned int startRule)
    : mTerminals(std::move(terminals)), mRules(std::move(rules)), mStartRule(startRule)
    {
    }

    std::unique_ptr<Grammar> ExtendedGrammar::makeGrammar() const
    {
        std::vector<Grammar::Rule> grammarRules;
        for(const auto &rule : mRules) {
            grammarRules.push_back(Grammar::Rule{rule.lhs});
        }

        for(unsigned int i=0; i<mRules.size(); i++) {
            populateRule(grammarRules, i, *mRules[i].rhs);
        }

        return std::make_unique<Grammar>(mTerminals, std::move(grammarRules), mStartRule);
    }

    void ExtendedGrammar::printRhsNode(const RhsNode &node) const {
        switch(node.type) {
            case RhsNode::Type::Symbol:
            {
                const RhsNodeSymbol &rhsNodeSymbol = static_cast<const RhsNodeSymbol&>(node);
                switch(rhsNodeSymbol.symbolType) {
                    case RhsNodeSymbol::SymbolType::Nonterminal:
                        std::cout << "<" << mRules[rhsNodeSymbol.index].lhs << ">";
                        break;
                    case RhsNodeSymbol::SymbolType::Terminal:
                        std::cout << mTerminals[rhsNodeSymbol.index];
                        break;
                }
                break;
            }

            case RhsNode::Type::Sequence:
            {
                const RhsNodeChildren &rhsNodeChildren = static_cast<const RhsNodeChildren&>(node);
                for(unsigned int i=0; i<rhsNodeChildren.children.size(); i++) {
                    printRhsNode(*rhsNodeChildren.children[i]);
                    if(i != rhsNodeChildren.children.size() - 1) {
                        std::cout << " ";
                    }
                }
                break;
            }

            case RhsNode::Type::OneOf:
            {
                const RhsNodeChildren &rhsNodeChildren = static_cast<const RhsNodeChildren&>(node);
                std::cout << "( ";
                for(unsigned int i=0; i<rhsNodeChildren.children.size(); i++) {
                    printRhsNode(*rhsNodeChildren.children[i]);
                    if(i != rhsNodeChildren.children.size() - 1) {
                        std::cout << " | ";
                    }
                }
                std::cout << " )";
                break;
            }

            case RhsNode::Type::ZeroOrOne:
            {
                const RhsNodeChild &rhsNodeChild = static_cast<const RhsNodeChild&>(node);
                if(rhsNodeChild.child->type == RhsNode::Type::Symbol) {
                    printRhsNode(*rhsNodeChild.child);
                    std::cout << " ?";
                } else {
                    std::cout << "( ";
                    printRhsNode(*rhsNodeChild.child);
                    std::cout << " ) ?";
                }
                break;
            }
            case RhsNode::Type::ZeroOrMore:
            {
                const RhsNodeChild &rhsNodeChild = static_cast<const RhsNodeChild&>(node);
                if(rhsNodeChild.child->type == RhsNode::Type::Symbol) {
                    printRhsNode(*rhsNodeChild.child);
                    std::cout << " *";
                } else {
                    std::cout << "( ";
                    printRhsNode(*rhsNodeChild.child);
                    std::cout << " ) *";
                }
                break;
            }
            case RhsNode::Type::OneOrMore:
            {
                const RhsNodeChild &rhsNodeChild = static_cast<const RhsNodeChild&>(node);
                if(rhsNodeChild.child->type == RhsNode::Type::Symbol) {
                    printRhsNode(*rhsNodeChild.child);
                    std::cout << " +";
                } else {
                    std::cout << "( ";
                    printRhsNode(*rhsNodeChild.child);
                    std::cout << " ) +";
                }
                break;
            }
        }
    }

    void ExtendedGrammar::print() const
    {
        for(const auto &rule : mRules) {
            std::cout << "<" << rule.lhs << ">: ";
            printRhsNode(*rule.rhs);
            std::cout << std::endl;
        }
    }

    void ExtendedGrammar::populateRule(std::vector<Grammar::Rule> &grammarRules, unsigned int index, const RhsNode &rhsNode) const
    {
        if(rhsNode.type == RhsNode::Type::OneOf) {
            const RhsNodeChildren &rhsNodeChildren = static_cast<const RhsNodeChildren&>(rhsNode);
            for(const auto &child : rhsNodeChildren.children) {
                Grammar::RHS grammarRhs;
                populateRhs(grammarRhs, *child, grammarRules, grammarRules[index].lhs);
                grammarRules[index].rhs.push_back(std::move(grammarRhs));
            }
        } else {
            Grammar::RHS grammarRhs;
            populateRhs(grammarRhs, rhsNode, grammarRules, grammarRules[index].lhs);
            grammarRules[index].rhs.push_back(std::move(grammarRhs));
        }
    }

    void ExtendedGrammar::populateRhs(Grammar::RHS &grammarRhs, const RhsNode &rhsNode, std::vector<Grammar::Rule> &grammarRules, const std::string &ruleName) const
    {
        if(rhsNode.type == RhsNode::Type::Sequence) {
            const RhsNodeChildren &rhsNodeChildren = static_cast<const RhsNodeChildren&>(rhsNode);
            for(const auto &child : rhsNodeChildren.children) {
                Grammar::Symbol grammarSymbol;
                populateSymbol(grammarSymbol, *child, grammarRules, ruleName);
                grammarRhs.push_back(std::move(grammarSymbol));
            }
        } else {
            Grammar::Symbol grammarSymbol;
            populateSymbol(grammarSymbol, rhsNode, grammarRules, ruleName);
            grammarRhs.push_back(std::move(grammarSymbol));
        }
    }

    std::string createSubRuleName(const std::string &ruleName, std::vector<Grammar::Rule> &grammarRules)
    {
        unsigned int n = 1;
        while(true) {
            std::stringstream ss;
            ss << ruleName << "." << n;
            std::string subRuleName = ss.str();
            
            bool found = false;
            for(const auto &rule : grammarRules) {
                if(rule.lhs == subRuleName) {
                    found = true;
                    break;
                }
            }

            if(found) {
                n++;
            } else {
                return subRuleName;
            }
        }
    }

    void ExtendedGrammar::populateSymbol(Grammar::Symbol &grammarSymbol, const RhsNode &rhsNode, std::vector<Grammar::Rule> &grammarRules, const std::string &ruleName) const
    {
        switch(rhsNode.type) {
            case RhsNode::Type::Symbol:
            {
                const RhsNodeSymbol &rhsNodeSymbol = static_cast<const RhsNodeSymbol&>(rhsNode);
                switch(rhsNodeSymbol.symbolType) {
                    case RhsNodeSymbol::SymbolType::Terminal: grammarSymbol.type = Grammar::Symbol::Type::Terminal; break;
                    case RhsNodeSymbol::SymbolType::Nonterminal: grammarSymbol.type = Grammar::Symbol::Type::Nonterminal; break;
                }
                grammarSymbol.index = rhsNodeSymbol.index;
                break;
            }

            case RhsNode::Type::ZeroOrOne:
            {
                const RhsNodeChild &rhsNodeChild = static_cast<const RhsNodeChild&>(rhsNode);
                grammarSymbol.type = Grammar::Symbol::Type::Nonterminal;
                grammarSymbol.index = (unsigned int)grammarRules.size();
                
                grammarRules.push_back(Grammar::Rule{createSubRuleName(ruleName, grammarRules)});
                Grammar::Rule &grammarRule = grammarRules[grammarSymbol.index];
                populateRule(grammarRules, grammarSymbol.index, *rhsNodeChild.child);
                
                Grammar::RHS rhs;
                rhs.push_back(Grammar::Symbol{Grammar::Symbol::Type::Epsilon, 0});
                grammarRules[grammarSymbol.index].rhs.push_back(std::move(rhs));
                break;
            }

            case RhsNode::Type::ZeroOrMore:
            {
                const RhsNodeChild &rhsNodeChild = static_cast<const RhsNodeChild&>(rhsNode);
                grammarSymbol.type = Grammar::Symbol::Type::Nonterminal;
                grammarSymbol.index = (unsigned int)grammarRules.size();
                
                grammarRules.push_back(Grammar::Rule{createSubRuleName(ruleName, grammarRules)});
                populateRule(grammarRules, grammarSymbol.index, *rhsNodeChild.child);
                for(auto &rhs : grammarRules[grammarSymbol.index].rhs) {
                    rhs.push_back(Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, grammarSymbol.index});
                }

                Grammar::RHS rhs;
                rhs.push_back(Grammar::Symbol{Grammar::Symbol::Type::Epsilon, 0});
                grammarRules[grammarSymbol.index].rhs.push_back(std::move(rhs));
                break;
            }

            case RhsNode::Type::OneOrMore:
            {
                const RhsNodeChild &rhsNodeChild = static_cast<const RhsNodeChild&>(rhsNode);
                grammarSymbol.type = Grammar::Symbol::Type::Nonterminal;
                grammarSymbol.index = (unsigned int)grammarRules.size();
                
                grammarRules.push_back(Grammar::Rule{createSubRuleName(ruleName, grammarRules)});
                
                unsigned int nextRuleIndex = (unsigned int)grammarRules.size();
                grammarRules.push_back(Grammar::Rule{createSubRuleName(ruleName, grammarRules)});
                
                populateRule(grammarRules, grammarSymbol.index, *rhsNodeChild.child);
                for(auto &rhs : grammarRules[grammarSymbol.index].rhs) {
                    rhs.push_back(Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, nextRuleIndex});
                }
                
                Grammar::Rule &nextRule = grammarRules[nextRuleIndex];
                nextRule.rhs = grammarRules[grammarSymbol.index].rhs;

                Grammar::RHS rhs;
                rhs.push_back(Grammar::Symbol{Grammar::Symbol::Type::Epsilon, 0});
                nextRule.rhs.push_back(std::move(rhs));
                break;
            }

            case RhsNode::Type::OneOf:
            {
                grammarSymbol.type = Grammar::Symbol::Type::Nonterminal;
                grammarSymbol.index = (unsigned int)grammarRules.size();
                
                grammarRules.push_back(Grammar::Rule{createSubRuleName(ruleName, grammarRules)});
                Grammar::Rule &grammarRule = grammarRules[grammarSymbol.index];
                populateRule(grammarRules, grammarSymbol.index, rhsNode);
                break;
            }
        }
    }
}