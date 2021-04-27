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

DefReader::DefReader(const std::string &filename)
{
    std::map<std::string, std::string> terminalDef;
    std::map<std::string, Rule> ruleDef;
    bool success = parseFile(filename, terminalDef, ruleDef);
    if(!success) {
        return;
    }

    std::map<std::string, unsigned int> terminalMap;
    std::map<std::string, unsigned int> anonymousTerminalMap;
    std::vector<std::string> terminals;
    std::vector<std::string> terminalNames;
    for(const auto &pair: terminalDef) {
        terminalNames.push_back(pair.first);
        terminals.push_back(pair.second);
        terminalMap[pair.first] = (unsigned int)(terminals.size() - 1);
    }

    std::vector<Grammar::Rule> rules;
    std::map<std::string, unsigned int> ruleMap;
    for(const auto &pair: ruleDef) {
        rules.push_back(Grammar::Rule());
        ruleMap[pair.first] = (unsigned int)(rules.size() - 1);
    }

    for(const auto &pair: ruleDef) {
        Grammar::Rule &rule = rules[ruleMap[pair.first]];
        rule.lhs = pair.first;
        for(const auto &r : pair.second) {
            Grammar::RHS rhs;
            for(const auto &s : r) {
                Grammar::Symbol symbol;
                if(s[0] == '<') {
                    symbol.type = Grammar::Symbol::Type::Nonterminal;
                    auto it = ruleMap.find(s);
                    if(it == ruleMap.end()) {
                        mParseError.message = "Unknown nonterminal " + s;
                        return;
                    } else {
                        symbol.index = it->second;
                        symbol.name = s;
                    }
                } else if(s[0] == '\'') {
                    std::string text = s.substr(1, s.size() - 2);
                    text = escape(text);
                    symbol.type = Grammar::Symbol::Type::Terminal;

                    auto it = anonymousTerminalMap.find(text);
                    if(it == anonymousTerminalMap.end()) {
                        terminals.push_back(text);
                        terminalNames.push_back(text);
                        symbol.index = anonymousTerminalMap[text] = (unsigned int)(terminals.size() - 1);
                    } else {
                        symbol.index = it->second;
                    }
                    symbol.name = text;
                } else if(s == "0") {
                    symbol.type = Grammar::Symbol::Type::Epsilon;
                    symbol.index = 0;
                    symbol.name = "0";
                } else {
                    symbol.type = Grammar::Symbol::Type::Terminal;
                    auto it = terminalMap.find(s);
                    if(it == terminalMap.end()) {
                        mParseError.message = "Unknown terminal " + s;
                        return;
                    } else {
                        symbol.index = it->second;
                        symbol.name = s;
                    }
                }
                rhs.push_back(symbol);
            }
            rule.rhs.push_back(rhs);
        }
    }

    Tokenizer::TokenValue endValue = (Tokenizer::TokenValue)terminals.size();
    auto it = ruleMap.find("<root>");
    if(it == ruleMap.end()) {
        mParseError.message = "No <root> nonterminal defined";
        return;
    } else {
        Grammar::Symbol endSymbol;
        endSymbol.type = Grammar::Symbol::Type::Terminal;
        endSymbol.index = endValue;
        endSymbol.name = "#";
        for(auto &rhs: rules[it->second].rhs) {
            rhs.push_back(endSymbol);
        }

        Tokenizer::Configuration configuration;
        for(unsigned int i=0; i<terminals.size(); i++) {
            Tokenizer::Pattern pattern;
            pattern.regex = std::move(terminals[i]);
            pattern.name = std::move(terminalNames[i]);
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
        mGrammar = std::make_unique<Grammar>(std::move(rules), it->second);
    }
}

struct DefToken {
    enum {
        Terminal,
        Nonterminal,
        Colon,
        Pipe,
        Literal,
        Regex,
        Newline,
        End
    };
};

struct StringData
{
    StringData(const std::string &t) : text(t) {}

    std::string text;
};

struct DefNode {
    enum class Type {
        List,
        Identifier,
        Literal,
        Pattern,
        Rule
    };

    DefNode(Type t) : type(t) {}
    template<typename ...Children> DefNode(Type t, Children&&... c) : type(t) {
        addChildren(std::forward<Children&&>(c)...);
    }
    void addChildren() {}
    template<typename ...Children> void addChildren(std::unique_ptr<DefNode>&& child, Children&&... others) {
        children.push_back(std::move(child));
        addChildren(std::forward<Children&&>(others)...);
    }

    Type type;
    std::vector<std::unique_ptr<DefNode>> children;
    std::string string;
};

bool DefReader::parseFile(const std::string &filename, std::map<std::string, std::string> &terminals, std::map<std::string, Rule> &rules)
{
    std::vector<Tokenizer::Configuration> configurations{
        Tokenizer::Configuration{std::vector<Tokenizer::Pattern>{
            Tokenizer::Pattern{"\\w+", "terminal", DefToken::Terminal,},
            Tokenizer::Pattern{"<\\w+>", "nonterminal", DefToken::Nonterminal},
            Tokenizer::Pattern{":", "colon", DefToken::Colon},
            Tokenizer::Pattern{"\\|", "pipe", DefToken::Pipe},
            Tokenizer::Pattern{"'[^']+'", "literal", DefToken::Literal},
            Tokenizer::Pattern{"\\s", "whitespace", Tokenizer::InvalidTokenValue}  
        }},
        Tokenizer::Configuration{std::vector<Tokenizer::Pattern>{
            Tokenizer::Pattern{"\\S+", "regex", DefToken::Regex},
            Tokenizer::Pattern{"\\s", "whitespace", Tokenizer::InvalidTokenValue},
        }}
    };

    Tokenizer tokenizer(std::move(configurations), DefToken::End, DefToken::Newline);

    std::vector<Grammar::Rule> grammarRules{
        Grammar::Rule{"root", std::vector<Grammar::RHS>{
            Grammar::RHS{
                Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, 1, "definitions"},
                Grammar::Symbol{Grammar::Symbol::Type::Terminal, DefToken::End, "#"},
            }
        }},
        Grammar::Rule{"definitions", std::vector<Grammar::RHS>{
            Grammar::RHS{
                Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, 2, "definitionlist"},
            }
        }},
        Grammar::Rule{"definitionlist", std::vector<Grammar::RHS>{
            Grammar::RHS{
                Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, 3, "pattern"},
                Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, 2, "definitionlist"}
            },
            Grammar::RHS{
                Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, 4, "rule"},
                Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, 2, "definitionlist"}
            },
            Grammar::RHS{
                Grammar::Symbol{Grammar::Symbol::Type::Terminal, DefToken::Newline, "newline"},
                Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, 2, "definitionlist"}
            },
            Grammar::RHS{
                Grammar::Symbol{Grammar::Symbol::Type::Epsilon, 0, ""}
            }
        }},
        Grammar::Rule{"pattern", std::vector<Grammar::RHS>{
            Grammar::RHS{
                Grammar::Symbol{Grammar::Symbol::Type::Terminal, DefToken::Terminal, "terminal"},
                Grammar::Symbol{Grammar::Symbol::Type::Terminal, DefToken::Colon, "colon"},
                Grammar::Symbol{Grammar::Symbol::Type::Terminal, DefToken::Regex, "regex"},
                Grammar::Symbol{Grammar::Symbol::Type::Terminal, DefToken::Newline, "newline"}
            }
        }},
        Grammar::Rule{"rule", std::vector<Grammar::RHS>{
            Grammar::RHS{
                Grammar::Symbol{Grammar::Symbol::Type::Terminal, DefToken::Nonterminal, "nonterminal"},
                Grammar::Symbol{Grammar::Symbol::Type::Terminal, DefToken::Colon, "colon"},
                Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, 5, "rhs"},
                Grammar::Symbol{Grammar::Symbol::Type::Terminal, DefToken::Newline, "newline"},
            }
        }},
        Grammar::Rule{"rhs", std::vector<Grammar::RHS>{
            Grammar::RHS{
                Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, 7, "rhsalt"},
                Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, 6, "rhsaltlist"},
            }
        }},
        Grammar::Rule{"rhsaltlist", std::vector<Grammar::RHS>{
            Grammar::RHS{
                Grammar::Symbol{Grammar::Symbol::Type::Terminal, DefToken::Pipe, "pipe"},
                Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, 7, "rhsalt"},
                Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, 6, "rhsaltlist"}
            },
            Grammar::RHS{
                Grammar::Symbol{Grammar::Symbol::Type::Epsilon, 0, ""}
            }
        }},
        Grammar::Rule{"rhsalt", std::vector<Grammar::RHS>{
            Grammar::RHS{
                Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, 8, "symbollist"},
            }
        }},
        Grammar::Rule{"symbollist", std::vector<Grammar::RHS>{
            Grammar::RHS{
                Grammar::Symbol{Grammar::Symbol::Type::Terminal, DefToken::Terminal, "terminal"},
                Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, 8, "symbollist"}
            },
            Grammar::RHS{
                Grammar::Symbol{Grammar::Symbol::Type::Terminal, DefToken::Nonterminal, "nonterminal"},
                Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, 8, "symbollist"}
            },
            Grammar::RHS{
                Grammar::Symbol{Grammar::Symbol::Type::Terminal, DefToken::Literal, "literal"},
                Grammar::Symbol{Grammar::Symbol::Type::Nonterminal, 8, "symbollist"}
            },
            Grammar::RHS{
                Grammar::Symbol{Grammar::Symbol::Type::Epsilon, 0, ""}
            }
        }},
    };

    Grammar grammar(std::move(grammarRules), 0);
    LLParser parser(grammar);
    
    std::ifstream file(filename);
    Tokenizer::Stream<StringData> stream(tokenizer, file);

    auto decorateToken = [](const std::string &text) -> std::unique_ptr<StringData> {
        return std::make_unique<StringData>(text);
    };
    stream.addDecorator("terminal", 0, decorateToken);
    stream.addDecorator("nonterminal", 0, decorateToken);
    stream.addDecorator("literal", 0, decorateToken);
    stream.addDecorator("regex", 1, decorateToken);

    LLParser::ParseSession<DefNode, StringData> session(parser);
    session.addMatchListener("pattern", 0, [&](unsigned int symbol) {
        if(symbol == 1) stream.setConfiguration(1);
        else if(symbol == 2) stream.setConfiguration(0);
    });

    auto terminalDecorator = [](const StringData &stringData) -> std::unique_ptr<DefNode> {
            std::unique_ptr<DefNode> node = std::make_unique<DefNode>(DefNode::Type::Identifier);
            node->string = stringData.text;  
            return node;  
    };
    session.addTerminalDecorator(DefToken::Terminal, terminalDecorator);
    session.addTerminalDecorator(DefToken::Nonterminal, terminalDecorator);
    session.addTerminalDecorator(DefToken::Literal, terminalDecorator);
    session.addTerminalDecorator(DefToken::Regex, terminalDecorator);

    session.addReducer("root", 0, [](LLParser::ParseItem<DefNode> *items, unsigned int numItems) -> std::unique_ptr<DefNode> {
        return std::move(items[0].data);
    });
    session.addReducer("definitions", 0, [](LLParser::ParseItem<DefNode> *items, unsigned int numItems) -> std::unique_ptr<DefNode> {
        std::unique_ptr<DefNode> node = std::make_unique<DefNode>(DefNode::Type::List);
        for(unsigned int i=0; i<numItems; i++) {
            if(items[i].data) {
                node->children.push_back(std::move(items[i].data));
            }
        }
        return node;
    });
    session.addReducer("pattern", 0, [](LLParser::ParseItem<DefNode> *items, unsigned int numItems) -> std::unique_ptr<DefNode> {
        return std::make_unique<DefNode>(DefNode::Type::Pattern, std::move(items[0].data), std::move(items[2].data));
    });
    session.addReducer("rule", 0, [](LLParser::ParseItem<DefNode> *items, unsigned int numItems) -> std::unique_ptr<DefNode> {
        return std::make_unique<DefNode>(DefNode::Type::Rule, std::move(items[0].data), std::move(items[2].data));
    });
    session.addReducer("rhs", 0, [](LLParser::ParseItem<DefNode> *items, unsigned int numItems) -> std::unique_ptr<DefNode> {
        std::unique_ptr<DefNode> node = std::make_unique<DefNode>(DefNode::Type::List);
        for(unsigned int i=0; i<numItems; i+=2) {
            node->children.push_back(std::move(items[i].data));
        }
        return node;
    });
    session.addReducer("rhsalt", 0, [](LLParser::ParseItem<DefNode> *items, unsigned int numItems) -> std::unique_ptr<DefNode> {
        std::unique_ptr<DefNode> node = std::make_unique<DefNode>(DefNode::Type::List);
        for(unsigned int i=0; i<numItems; i++) {
            node->children.push_back(std::move(items[i].data));
        }
        return node;
    });
    
    std::unique_ptr<DefNode> node = session.parse(stream);

    for(const auto &definition: node->children) {
        switch(definition->type) {
            case DefNode::Type::Pattern:
            {
                terminals[definition->children[0]->string] = definition->children[1]->string;
                break;
            }

            case DefNode::Type::Rule:
            {
                std::string name = definition->children[0]->string;
                for(const auto &alt : definition->children[1]->children) {
                    std::vector<std::string> symbols;
                    for(const auto &symbol : alt->children) {
                        symbols.push_back(symbol->string);
                    }
                    rules[name].push_back(std::move(symbols));
                }
                break;
            }

        } 
    }

    return true;
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