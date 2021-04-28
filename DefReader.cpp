#include "DefReader.hpp"
#include "LLParser.hpp"

#include <fstream>
#include <vector>
#include <iostream>

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

struct StringData
{
    StringData(const std::string &t) : text(t) {}

    std::string text;
};

struct DefNode {
    enum class Type {
        List,
        Terminal,
        Nonterminal,
        Literal,
        Epsilon,
        Regex,
        Pattern,
        Rule
    };

    template<typename ...Children> DefNode(Type t, Children&&... c) : type(t) {
        children.reserve(sizeof...(c));
        (children.push_back(std::move(c)), ...);
    }
    DefNode(Type t, const std::string &s) : type(t), string(s) {}

    Type type;
    std::vector<std::unique_ptr<DefNode>> children;
    std::string string;
};

DefReader::DefReader(const std::string &filename)
{
    std::vector<std::string> grammarTerminals{
        /* 0 */ "epsilon",
        /* 1 */ "terminal",
        /* 2 */ "nonterminal",
        /* 3 */ "colon",
        /* 4 */ "pipe",
        /* 5 */ "literal",
        /* 6 */ "regex",
        /* 7 */ "newline",
        /* 8 */ "end"
    };

    std::vector<Tokenizer::Configuration> configurations{
        Tokenizer::Configuration{std::vector<Tokenizer::Pattern>{
            Tokenizer::Pattern{"0", "epsilon", 0},
            Tokenizer::Pattern{"\\w+", "terminal", 1},
            Tokenizer::Pattern{"<\\w+>", "nonterminal", 2},
            Tokenizer::Pattern{":", "colon", 3},
            Tokenizer::Pattern{"\\|", "pipe", 4},
            Tokenizer::Pattern{"'[^']+'", "literal", 5},
            Tokenizer::Pattern{"\\s", "whitespace", Tokenizer::InvalidTokenValue}  
        }},
        Tokenizer::Configuration{std::vector<Tokenizer::Pattern>{
            Tokenizer::Pattern{"\\S+", "regex", 6},
            Tokenizer::Pattern{"\\s", "whitespace", Tokenizer::InvalidTokenValue},
        }}
    };

    Tokenizer tokenizer(std::move(configurations), 8, 7);

    std::vector<Grammar::Rule> grammarRules{
        // 0:
        Grammar::Rule{"root", std::vector<Grammar::RHS>{
            Grammar::RHS{
                Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, 1},
                Grammar::Symbol{Grammar::Symbol::Type::Terminal, 8},
            }
        }},

        // 1:
        Grammar::Rule{"definitions", std::vector<Grammar::RHS>{
            Grammar::RHS{
                Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, 2},
            }
        }},

        // 2:
        Grammar::Rule{"definitionlist", std::vector<Grammar::RHS>{
            Grammar::RHS{
                Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, 3},
                Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, 2}
            },
            Grammar::RHS{
                Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, 4},
                Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, 2}
            },
            Grammar::RHS{
                Grammar::Symbol{Grammar::Symbol::Type::Terminal, 7},
                Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, 2}
            },
            Grammar::RHS{
                Grammar::Symbol{Grammar::Symbol::Type::Epsilon, 0}
            }
        }},

        // 3:
        Grammar::Rule{"pattern", std::vector<Grammar::RHS>{
            Grammar::RHS{
                Grammar::Symbol{Grammar::Symbol::Type::Terminal, 1},
                Grammar::Symbol{Grammar::Symbol::Type::Terminal, 3},
                Grammar::Symbol{Grammar::Symbol::Type::Terminal, 6},
                Grammar::Symbol{Grammar::Symbol::Type::Terminal, 7}
            }
        }},

        // 4:
        Grammar::Rule{"rule", std::vector<Grammar::RHS>{
            Grammar::RHS{
                Grammar::Symbol{Grammar::Symbol::Type::Terminal, 2},
                Grammar::Symbol{Grammar::Symbol::Type::Terminal, 3},
                Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, 5},
                Grammar::Symbol{Grammar::Symbol::Type::Terminal, 7},
            }
        }},

        // 5:
        Grammar::Rule{"rhs", std::vector<Grammar::RHS>{
            Grammar::RHS{
                Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, 7},
                Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, 6},
            }
        }},

        // 6:
        Grammar::Rule{"rhsaltlist", std::vector<Grammar::RHS>{
            Grammar::RHS{
                Grammar::Symbol{Grammar::Symbol::Type::Terminal, 4},
                Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, 7},
                Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, 6}
            },
            Grammar::RHS{
                Grammar::Symbol{Grammar::Symbol::Type::Epsilon, 0}
            }
        }},

        // 7:
        Grammar::Rule{"rhsalt", std::vector<Grammar::RHS>{
            Grammar::RHS{
                Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, 8},
            }
        }},

        // 8:
        Grammar::Rule{"symbollist", std::vector<Grammar::RHS>{
            Grammar::RHS{
                Grammar::Symbol{Grammar::Symbol::Type::Terminal, 1},
                Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, 8}
            },
            Grammar::RHS{
                Grammar::Symbol{Grammar::Symbol::Type::Terminal, 2},
                Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, 8}
            },
            Grammar::RHS{
                Grammar::Symbol{Grammar::Symbol::Type::Terminal, 5},
                Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, 8}
            },
            Grammar::RHS{
                Grammar::Symbol{Grammar::Symbol::Type::Terminal, 0},
                Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, 8}
            },
            Grammar::RHS{
                Grammar::Symbol{Grammar::Symbol::Type::Epsilon, 0}
            }
        }},
    };

    Grammar grammar(std::move(grammarTerminals), std::move(grammarRules), 0);
    LLParser parser(grammar);
    
    std::ifstream file(filename);
    Tokenizer::Stream<StringData> stream(tokenizer, file);

    stream.addDecorator("terminal", 0, [](const std::string &text) {
        return std::make_unique<StringData>(text);
    });
    stream.addDecorator("nonterminal", 0, [](const std::string &text) {
        return std::make_unique<StringData>(text.substr(1, text.size() - 2));
    });
    stream.addDecorator("literal", 0, [](const std::string &text) {
        return std::make_unique<StringData>(text.substr(1, text.size() - 2));
    });
    stream.addDecorator("regex", 1, [](const std::string &text) {
        return std::make_unique<StringData>(text);
    });

    LLParser::ParseSession<DefNode, StringData> session(parser);
    session.addMatchListener("pattern", 0, [&](unsigned int symbol) {
        if(symbol == 1) stream.setConfiguration(1);
        else if(symbol == 2) stream.setConfiguration(0);
    });

    session.addTerminalDecorator("epsilon", [](const StringData &stringData) {
        return std::make_unique<DefNode>(DefNode::Type::Epsilon);
    });
    session.addTerminalDecorator("terminal", [](const StringData &stringData) {
        return std::make_unique<DefNode>(DefNode::Type::Terminal, stringData.text);
    });
    session.addTerminalDecorator("nonterminal", [](const StringData &stringData) {
        return std::make_unique<DefNode>(DefNode::Type::Nonterminal, stringData.text);
    });
    session.addTerminalDecorator("literal", [](const StringData &stringData) {
        return std::make_unique<DefNode>(DefNode::Type::Literal, stringData.text);
    });
    session.addTerminalDecorator("regex", [](const StringData &stringData) {
        return std::make_unique<DefNode>(DefNode::Type::Regex, stringData.text);
    });

    session.addReducer("root", 0, [](LLParser::ParseItem<DefNode> *items, unsigned int numItems) {
        return std::move(items[0].data);
    });
    session.addReducer("definitions", 0, [](LLParser::ParseItem<DefNode> *items, unsigned int numItems) {
        std::unique_ptr<DefNode> node = std::make_unique<DefNode>(DefNode::Type::List);
        for(unsigned int i=0; i<numItems; i++) {
            if(items[i].data) {
                node->children.push_back(std::move(items[i].data));
            }
        }
        return node;
    });
    session.addReducer("pattern", 0, [](LLParser::ParseItem<DefNode> *items, unsigned int numItems) {
        return std::make_unique<DefNode>(DefNode::Type::Pattern, std::move(items[0].data), std::move(items[2].data));
    });
    session.addReducer("rule", 0, [](LLParser::ParseItem<DefNode> *items, unsigned int numItems) {
        return std::make_unique<DefNode>(DefNode::Type::Rule, std::move(items[0].data), std::move(items[2].data));
    });
    session.addReducer("rhs", 0, [](LLParser::ParseItem<DefNode> *items, unsigned int numItems) {
        std::unique_ptr<DefNode> node = std::make_unique<DefNode>(DefNode::Type::List);
        for(unsigned int i=0; i<numItems; i+=2) {
            node->children.push_back(std::move(items[i].data));
        }
        return node;
    });
    session.addReducer("rhsalt", 0, [](LLParser::ParseItem<DefNode> *items, unsigned int numItems) {
        std::unique_ptr<DefNode> node = std::make_unique<DefNode>(DefNode::Type::List);
        for(unsigned int i=0; i<numItems; i++) {
            node->children.push_back(std::move(items[i].data));
        }
        return node;
    });
    
    std::unique_ptr<DefNode> node = session.parse(stream);

    std::map<std::string, unsigned int> terminalMap;
    std::map<std::string, unsigned int> anonymousTerminalMap;
    std::vector<std::string> terminals;
    std::vector<std::string> terminalNames;

    for(const auto &definition: node->children) {
        if(definition->type == DefNode::Type::Pattern) {
            terminals.push_back(definition->children[1]->string);
            terminalNames.push_back(definition->children[0]->string);
            terminalMap[definition->children[0]->string] = (unsigned int)(terminals.size() - 1);
        }
    }

    std::vector<Grammar::Rule> rules;
    std::map<std::string, unsigned int> ruleMap;
    for(const auto &definition: node->children) {
        if(definition->type == DefNode::Type::Rule) {
            rules.push_back(Grammar::Rule());
            ruleMap[definition->children[0]->string] = (unsigned int)(rules.size() - 1);
        }
    }

    for(const auto &definition: node->children) {
        if(definition->type == DefNode::Type::Rule) {
            std::string name = definition->children[0]->string;
            Grammar::Rule &rule = rules[ruleMap[name]];
            rule.lhs = name;
        
            for(const auto &alt : definition->children[1]->children) {
                Grammar::RHS rhs;
                for(const auto &sym : alt->children) {
                    Grammar::Symbol symbol;
                    switch(sym->type) {
                        case DefNode::Type::Terminal:
                        {
                            symbol.type = Grammar::Symbol::Type::Terminal;
                            auto it = terminalMap.find(sym->string);
                            if(it == terminalMap.end()) {
                                mParseError.message = "Unknown terminal " + sym->string;
                                return;
                            } else {
                                symbol.index = it->second;
                            }
                            break;
                        }

                        case DefNode::Type::Nonterminal:
                        {
                            symbol.type = Grammar::Symbol::Type::Nonterminal;
                            auto it = ruleMap.find(sym->string);
                            if(it == ruleMap.end()) {
                                mParseError.message = "Unknown nonterminal " + sym->string;
                                return;
                            } else {
                                symbol.index = it->second;
                            }
                            break;
                        }

                        case DefNode::Type::Literal:
                        {
                            std::string text = escape(sym->string);
                            symbol.type = Grammar::Symbol::Type::Terminal;

                            auto it = anonymousTerminalMap.find(text);
                            if(it == anonymousTerminalMap.end()) {
                                terminals.push_back(text);
                                terminalNames.push_back(text);
                                symbol.index = anonymousTerminalMap[text] = (unsigned int)(terminals.size() - 1);
                            } else {
                                symbol.index = it->second;
                            }
                            break;
                        }

                        case DefNode::Type::Epsilon:
                        {
                            symbol.type = Grammar::Symbol::Type::Epsilon;
                            symbol.index = 0;
                            break;
                        }
                    }
                    rhs.push_back(symbol);
                }
                rule.rhs.push_back(std::move(rhs));
            }   
        }
    }

    Tokenizer::TokenValue endValue = (Tokenizer::TokenValue)terminals.size();
    terminals.push_back("");
    terminalNames.push_back("END");

    auto it = ruleMap.find("root");
    if(it == ruleMap.end()) {
        mParseError.message = "No <root> nonterminal defined";
        return;
    } else {
        Grammar::Symbol endSymbol;
        endSymbol.type = Grammar::Symbol::Type::Terminal;
        endSymbol.index = endValue;
        for(auto &rhs: rules[it->second].rhs) {
            rhs.push_back(endSymbol);
        }

        Tokenizer::Configuration configuration;
        for(unsigned int i=0; i<terminals.size(); i++) {
            Tokenizer::Pattern pattern;
            pattern.regex = terminals[i];
            pattern.name = terminalNames[i];
            if(pattern.name == "IGNORE") {
                pattern.value = Tokenizer::InvalidTokenValue;
            } else {
                pattern.value = i;
            }
            configuration.patterns.push_back(std::move(pattern));
        }
        std::vector<Tokenizer::Configuration> configurations;
        configurations.push_back(std::move(configuration));
        mTokenizer = std::make_unique<Tokenizer>(std::move(configurations), endValue, Tokenizer::InvalidTokenValue);
        mGrammar = std::make_unique<Grammar>(std::move(terminalNames), std::move(rules), it->second);
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

const Grammar &DefReader::grammar() const
{
    return *mGrammar;
}