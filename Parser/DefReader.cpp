#include "Parser/DefReader.hpp"
#include "Parser/Impl/LL.hpp"
#include "Parser/ExtendedGrammar.hpp"

#include <fstream>
#include <vector>
#include <iostream>

namespace Parser
{
    DefReader::DefReader(const std::string &filename)
    {
        std::unique_ptr<DefNode> node = parseFile(filename);
        if(!node) {
            return;
        }

        for(const auto &definition: node->children) {
            if(definition->type == DefNode::Type::Pattern) {
                mTerminals.push_back(definition->children[1]->string);
                mTerminalNames.push_back(definition->children[0]->string);
                mTerminalMap[definition->children[0]->string] = (unsigned int)(mTerminals.size() - 1);
            }
        }

        for(const auto &definition: node->children) {
            if(definition->type == DefNode::Type::Rule) {
                mRules.push_back(ExtendedGrammar::Rule());
                mRuleMap[definition->children[0]->string] = (unsigned int)(mRules.size() - 1);
            }
        }

        for(const auto &definition: node->children) {
            if(definition->type == DefNode::Type::Rule) {
                std::string name = definition->children[0]->string;
                ExtendedGrammar::Rule &rule = mRules[mRuleMap[name]];
                rule.lhs = name;
                rule.rhs = createRhsNode(*definition->children[1]);
                if(!rule.rhs) {
                    return;
                }
            }
        }

        Tokenizer::TokenValue endValue = (Tokenizer::TokenValue)mTerminals.size();
        mTerminals.push_back("");
        mTerminalNames.push_back("END");

        auto it = mRuleMap.find("root");
        if(it == mRuleMap.end()) {
            mParseError.message = "No <root> nonterminal defined";
            return;
        } else {
            ExtendedGrammar::Rule &rule = mRules[it->second];
            std::unique_ptr<ExtendedGrammar::RhsNode> endNode = std::make_unique<ExtendedGrammar::RhsNodeSymbol>(ExtendedGrammar::RhsNodeSymbol::SymbolType::Terminal, endValue);
            if(rule.rhs->type == ExtendedGrammar::RhsNode::Type::Sequence) {
                static_cast<ExtendedGrammar::RhsNodeChildren&>(*rule.rhs).children.push_back(std::move(endNode));
            } else {
                std::unique_ptr<ExtendedGrammar::RhsNode> newNode = std::make_unique<ExtendedGrammar::RhsNodeChildren>(ExtendedGrammar::RhsNode::Type::Sequence, std::move(rule.rhs), std::move(endNode));
                rule.rhs = std::move(newNode);
            }

            Tokenizer::Configuration configuration;
            for(unsigned int i=0; i<mTerminals.size(); i++) {
                Tokenizer::Pattern pattern;
                pattern.regex = mTerminals[i];
                pattern.name = mTerminalNames[i];
                if(pattern.name == "IGNORE") {
                    pattern.value = Tokenizer::kInvalidTokenValue;
                } else {
                    pattern.value = i;
                }
                configuration.patterns.push_back(std::move(pattern));
            }
            std::vector<Tokenizer::Configuration> configurations;
            configurations.push_back(std::move(configuration));
            mTokenizer = std::make_unique<Tokenizer>(std::move(configurations), endValue, Tokenizer::kInvalidTokenValue);
            ExtendedGrammar extendedGrammar(std::move(mTerminalNames), std::move(mRules), it->second);
            mGrammar = extendedGrammar.makeGrammar();
        }
    }

    bool DefReader::valid() const
    {
        return mTokenizer && mGrammar;
    }

    const DefReader::ParseError &DefReader::parseError() const
    {
        return mParseError;
    }

    const Tokenizer &DefReader::tokenizer() const
    {
        return *mTokenizer;
    }

    const Parser::Grammar &DefReader::grammar() const
    {
        return *mGrammar;
    }

    std::unique_ptr<ExtendedGrammar::RhsNode> ZeroOrMore(std::unique_ptr<ExtendedGrammar::RhsNode> node) {
        return std::make_unique<ExtendedGrammar::RhsNodeChild>(ExtendedGrammar::RhsNode::Type::ZeroOrMore, std::move(node));
    };

    std::unique_ptr<ExtendedGrammar::RhsNode> OneOrMore(std::unique_ptr<ExtendedGrammar::RhsNode> node) {
        return std::make_unique<ExtendedGrammar::RhsNodeChild>(ExtendedGrammar::RhsNode::Type::OneOrMore, std::move(node));
    };

    std::unique_ptr<ExtendedGrammar::RhsNode> ZeroOrOne(std::unique_ptr<ExtendedGrammar::RhsNode> node) {
        return std::make_unique<ExtendedGrammar::RhsNodeChild>(ExtendedGrammar::RhsNode::Type::ZeroOrOne, std::move(node));
    };

    template<typename...Args> std::unique_ptr<ExtendedGrammar::RhsNode> OneOf(Args... nodes) {
        return std::make_unique<ExtendedGrammar::RhsNodeChildren>(ExtendedGrammar::RhsNode::Type::OneOf, std::forward<Args>(nodes)...);
    };

    template<typename...Args> std::unique_ptr<ExtendedGrammar::RhsNode> Sequence(Args... nodes) {
        return std::make_unique<ExtendedGrammar::RhsNodeChildren>(ExtendedGrammar::RhsNode::Type::Sequence, std::forward<Args>(nodes)...);
    };

    void DefReader::createDefGrammar()
    {
        std::vector<std::string> grammarTerminals{
            "epsilon",
            "terminal",
            "nonterminal",
            "colon",
            "pipe",
            "lparen",
            "rparen",
            "star",
            "plus",
            "question",
            "literal",
            "regex",
            "newline",
            "end"
        };

        auto tokenIndex = [&](const std::string &name) {
            for(unsigned int i=0; i<grammarTerminals.size(); i++) {
                if(grammarTerminals[i] == name) {
                    return i;
                }
            }
            return Tokenizer::kInvalidTokenValue;
        };

        auto pattern = [&](const std::string &regex, const std::string &name) {
            return Tokenizer::Pattern{regex, name, tokenIndex(name)};
        };

        std::vector<Tokenizer::Configuration> configurations{
            Tokenizer::Configuration{std::vector<Tokenizer::Pattern>{
                pattern("0", "epsilon"),
                pattern("\\w+", "terminal"),
                pattern("<\\w+>", "nonterminal"),
                pattern(":", "colon"),
                pattern("\\|", "pipe"),
                pattern("\\(", "lparen"),
                pattern("\\)", "rparen"),
                pattern("\\+", "plus"),
                pattern("\\*", "star"),
                pattern("\\?", "question"),
                pattern("'[^']+'", "literal"),
                pattern("\\s", "whitespace")  
            }},
            Tokenizer::Configuration{std::vector<Tokenizer::Pattern>{
                pattern("\\S+", "regex"),
                pattern("\\s", "whitespace")
            }}
        };

        mDefTokenizer = std::make_unique<Tokenizer>(std::move(configurations), tokenIndex("end"), tokenIndex("newline"));

        std::vector<ExtendedGrammar::Rule> grammarRules;
        grammarRules.push_back(ExtendedGrammar::Rule{"root"});
        grammarRules.push_back(ExtendedGrammar::Rule{"definitions"});
        grammarRules.push_back(ExtendedGrammar::Rule{"pattern"});
        grammarRules.push_back(ExtendedGrammar::Rule{"rule"});
        grammarRules.push_back(ExtendedGrammar::Rule{"rhs"});
        grammarRules.push_back(ExtendedGrammar::Rule{"rhsSuffix"});
        grammarRules.push_back(ExtendedGrammar::Rule{"rhsSymbol"});

        auto ruleIndex = [&](const std::string &name) {
            for(unsigned int i=0; i<grammarRules.size(); i++) {
                if(grammarRules[i].lhs == name) {
                    return i;
                }
            }
            return UINT_MAX;
        };

        auto T = [&](const std::string &name) -> std::unique_ptr<ExtendedGrammar::RhsNode>  {
            return std::make_unique<ExtendedGrammar::RhsNodeSymbol>(ExtendedGrammar::RhsNodeSymbol::SymbolType::Terminal, tokenIndex(name));
        };

        auto N = [&](const std::string &name) -> std::unique_ptr<ExtendedGrammar::RhsNode>  {
            return std::make_unique<ExtendedGrammar::RhsNodeSymbol>(ExtendedGrammar::RhsNodeSymbol::SymbolType::Nonterminal, ruleIndex(name));
        };

        auto SetRule = [&](const std::string &name, std::unique_ptr<ExtendedGrammar::RhsNode> node) {
            grammarRules[ruleIndex(name)].rhs = std::move(node);
        };

        SetRule("root", Sequence(N("definitions"), T("end")));
        SetRule("definitions", ZeroOrMore(OneOf(N("pattern"), N("rule"), T("newline"))));
        SetRule("pattern", Sequence(T("terminal"), T("colon"), T("regex"), T("newline")));
        SetRule("rule",
            Sequence(
                T("nonterminal"),
                T("colon"),
                N("rhs"),
                ZeroOrMore(Sequence(T("pipe"), N("rhs"))),
                T("newline")
            )
        );
        SetRule("rhs", OneOrMore(N("rhsSuffix")));
        SetRule("rhsSuffix", 
            Sequence(
                N("rhsSymbol"),
                ZeroOrMore(
                    OneOf(T("star"), T("plus"), T("question"))
                )
            )
        );
        SetRule("rhsSymbol",
            OneOf(
                T("terminal"),
                T("nonterminal"),
                T("literal"),
                Sequence(
                    T("lparen"), N("rhs"), ZeroOrMore(Sequence(T("pipe"), N("rhs"))), T("rparen")
                )
            )
        );

        ExtendedGrammar extendedGrammar(std::move(grammarTerminals), std::move(grammarRules), 0);
        mDefGrammar = extendedGrammar.makeGrammar();
    }

    std::unique_ptr<DefReader::DefNode> DefReader::parseFile(const std::string &filename)
    {
        createDefGrammar();
        Parser::Impl::LL parser(*mDefGrammar);
        
        std::ifstream file(filename);
        Tokenizer::Stream stream(*mDefTokenizer, file);

        Parser::Impl::LL::ParseSession<DefNode> session(parser);
        session.addMatchListener("pattern", [&](unsigned int symbol) {
            if(symbol == 1) stream.setConfiguration(1);
            else if(symbol == 2) stream.setConfiguration(0);
        });

        session.addTerminalDecorator("terminal", [](const Tokenizer::Token &token) {
            return std::make_unique<DefNode>(DefNode::Type::Terminal, token.text, token.line);
        });
        session.addTerminalDecorator("nonterminal", [](const Tokenizer::Token &token) {
            return std::make_unique<DefNode>(DefNode::Type::Nonterminal, token.text.substr(1, token.text.size() - 2), token.line);
        });
        session.addTerminalDecorator("literal", [](const Tokenizer::Token &token) {
            return std::make_unique<DefNode>(DefNode::Type::Literal, token.text.substr(1, token.text.size() - 2), token.line);
        });
        session.addTerminalDecorator("regex", [](const Tokenizer::Token &token) {
            return std::make_unique<DefNode>(DefNode::Type::Regex, token.text, token.line);
        });

        session.addReducer("root", [](auto begin, auto end) {
            return std::move(begin->data);
        });
        session.addReducer("definitions", [](auto begin, auto end) {
            std::unique_ptr<DefNode> node = std::make_unique<DefNode>(DefNode::Type::List);
            for(auto it = begin; it != end; ++it) {
                if(it->data) {
                    node->children.push_back(std::move(it->data));
                }
            }
            return node;
        });
        session.addReducer("pattern", [](auto begin, auto end) {
            auto it = begin;
            std::unique_ptr<DefNode> name = std::move(it->data);
            it += 2;
            std::unique_ptr<DefNode> regex = std::move(it->data);
            return std::make_unique<DefNode>(DefNode::Type::Pattern, std::move(name), std::move(regex));
        });
        session.addReducer("rule", [](auto begin, auto end) {
            auto it = begin;
            std::unique_ptr<DefNode> lhs = std::move(it->data);
            it += 2;
            
            std::unique_ptr<DefNode> rhs = std::make_unique<DefNode>(DefNode::Type::RhsOneOf);
            while(it != end) {
                rhs->children.push_back(std::move(it->data));
                it += 2;
            }
            if(rhs->children.size() == 1) {
                rhs = std::move(rhs->children[0]);
            }
            return std::make_unique<DefNode>(DefNode::Type::Rule, std::move(lhs), std::move(rhs));
        });
        session.addReducer("rhs", [](auto begin, auto end) {
            std::unique_ptr<DefNode> node = std::make_unique<DefNode>(DefNode::Type::RhsSequence);
            for(auto it = begin; it != end; ++it) {
                node->children.push_back(std::move(it->data));
            }
            if(node->children.size() == 1) {
                node = std::move(node->children[0]);
            }
            return node;
        });
        unsigned int star = mDefGrammar->terminalIndex("star");
        unsigned int plus = mDefGrammar->terminalIndex("plus");
        unsigned int question = mDefGrammar->terminalIndex("question");
        session.addReducer("rhsSuffix", [&](auto begin, auto end) {
            auto it = begin;
            std::unique_ptr<DefNode> node = std::move(it->data);
            ++it;
            for(; it != end; ++it) {
                if(it->index == star) {
                    node = std::make_unique<DefNode>(DefNode::Type::RhsZeroOrMore, std::move(node));
                } else if(it->index == plus) {
                    node = std::make_unique<DefNode>(DefNode::Type::RhsOneOrMore, std::move(node));
                } else if(it->index == question) {
                    node = std::make_unique<DefNode>(DefNode::Type::RhsZeroOrOne, std::move(node));
                }
            }
            return node;
        });
        unsigned int lparen = mDefGrammar->terminalIndex("lparen");
        unsigned int rparen = mDefGrammar->terminalIndex("rparen");
        session.addReducer("rhsSymbol", [&](auto begin, auto end) {
            std::unique_ptr<DefNode> node;
            auto it = begin;
            if(it->index == lparen) {
                ++it;
                node = std::make_unique<DefNode>(DefNode::Type::RhsOneOf);
                while(true) {
                    node->children.push_back(std::move(it->data));
                    ++it;
                    if(it->index == rparen) {
                        break;
                    }
                    ++it;
                }
                if(node->children.size() == 1) {
                    node = std::move(node->children[0]);
                }
            } else {
                node = std::move(it->data);
            }
            return node;
        });

        std::unique_ptr<DefNode> node = session.parse(stream);
        if(!node) {
            mParseError.line = stream.nextToken().line;
            mParseError.message =  "Unexpected symbol " + stream.nextToken().text;
        }
        return node;
    }

    std::string escape(const std::string &input)
    {
        std::string result;

        for(char c : input) {
            switch(c) {
                case ' ': result.append("\\s"); break;
                case '+':
                case '*':
                case '?':
                case '(':
                case ')':
                case '[':
                case ']': result.push_back('\\');
                default: result.push_back(c); break;
            }
        }

        return result;
    }

    std::unique_ptr<ExtendedGrammar::RhsNode> DefReader::createRhsNode(const DefNode &defNode)
    {
        switch(defNode.type) {
            case DefNode::Type::Terminal:
            {
                unsigned int index = UINT_MAX;
                auto it = mTerminalMap.find(defNode.string);
                if(it == mTerminalMap.end()) {
                    mParseError.line = defNode.line;
                    mParseError.message = "Unknown terminal " + defNode.string;
                    return nullptr;
                } else {
                    index = it->second;
                }
                return std::make_unique<ExtendedGrammar::RhsNodeSymbol>(ExtendedGrammar::RhsNodeSymbol::SymbolType::Terminal, index);
            }

            case DefNode::Type::Nonterminal:
            {
                unsigned int index = UINT_MAX;
                auto it = mRuleMap.find(defNode.string);
                if(it == mRuleMap.end()) {
                    mParseError.line = defNode.line;
                    mParseError.message = "Unknown nonterminal " + defNode.string;
                    return nullptr;
                } else {
                    index = it->second;
                }
                return std::make_unique<ExtendedGrammar::RhsNodeSymbol>(ExtendedGrammar::RhsNodeSymbol::SymbolType::Nonterminal, index);
            }

            case DefNode::Type::Literal:
            {
                const std::string &text = defNode.string;
                std::string escapedText = escape(text);
                unsigned int index;

                auto it = mAnonymousTerminalMap.find(text);
                if(it == mAnonymousTerminalMap.end()) {
                    mTerminals.push_back(escapedText);
                    mTerminalNames.push_back(text);
                    index = mAnonymousTerminalMap[text] = (unsigned int)(mTerminals.size() - 1);
                } else {
                    index = it->second;
                }
                return std::make_unique<ExtendedGrammar::RhsNodeSymbol>(ExtendedGrammar::RhsNodeSymbol::SymbolType::Terminal, index);
            }

            case DefNode::Type::RhsSequence:
            {
                std::unique_ptr<ExtendedGrammar::RhsNodeChildren> node = std::make_unique<ExtendedGrammar::RhsNodeChildren>(ExtendedGrammar::RhsNode::Type::Sequence);
                for(auto &child : defNode.children) {
                    std::unique_ptr<ExtendedGrammar::RhsNode> childRhsNode = createRhsNode(*child);
                    if(!childRhsNode) {
                        return nullptr;
                    }
                    node->children.push_back(std::move(childRhsNode));
                }
                return node;
            }

            case DefNode::Type::RhsOneOf:
            {
                std::unique_ptr<ExtendedGrammar::RhsNodeChildren> node = std::make_unique<ExtendedGrammar::RhsNodeChildren>(ExtendedGrammar::RhsNode::Type::OneOf);
                for(auto &child : defNode.children) {
                    std::unique_ptr<ExtendedGrammar::RhsNode> childRhsNode = createRhsNode(*child);
                    if(!childRhsNode) {
                        return nullptr;
                    }
                    node->children.push_back(std::move(childRhsNode));
                }
                return node;
            }

            case DefNode::Type::RhsZeroOrMore:
            {
                std::unique_ptr<ExtendedGrammar::RhsNode> child = createRhsNode(*defNode.children[0]);
                if(!child) {
                    return nullptr;
                }
                return std::make_unique<ExtendedGrammar::RhsNodeChild>(ExtendedGrammar::RhsNode::Type::ZeroOrMore, std::move(child));
            }

            case DefNode::Type::RhsOneOrMore:
            {
                std::unique_ptr<ExtendedGrammar::RhsNode> child = createRhsNode(*defNode.children[0]);
                if(!child) {
                    return nullptr;
                }
                return std::make_unique<ExtendedGrammar::RhsNodeChild>(ExtendedGrammar::RhsNode::Type::OneOrMore, std::move(child));
            }

            case DefNode::Type::RhsZeroOrOne:
            {
                std::unique_ptr<ExtendedGrammar::RhsNode> child = createRhsNode(*defNode.children[0]);
                if(!child) {
                    return nullptr;
                }
                return std::make_unique<ExtendedGrammar::RhsNodeChild>(ExtendedGrammar::RhsNode::Type::ZeroOrOne, std::move(child));
            }
        }

        return nullptr;
    };
}