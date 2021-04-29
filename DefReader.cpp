#include "DefReader.hpp"
#include "LLParser.hpp"
#include "ExtendedGrammar.hpp"

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

std::unique_ptr<ExtendedGrammar::RhsNode> ZeroOrMore(std::unique_ptr<ExtendedGrammar::RhsNode> &&node) {
    return std::make_unique<ExtendedGrammar::RhsNodeChild>(ExtendedGrammar::RhsNode::Type::ZeroOrMore, std::move(node));
};

std::unique_ptr<ExtendedGrammar::RhsNode> OneOrMore(std::unique_ptr<ExtendedGrammar::RhsNode> &&node) {
    return std::make_unique<ExtendedGrammar::RhsNodeChild>(ExtendedGrammar::RhsNode::Type::OneOrMore, std::move(node));
};

std::unique_ptr<ExtendedGrammar::RhsNode> ZeroOrOne(std::unique_ptr<ExtendedGrammar::RhsNode> &&node) {
    return std::make_unique<ExtendedGrammar::RhsNodeChild>(ExtendedGrammar::RhsNode::Type::ZeroOrOne, std::move(node));
};

template<typename...Args> std::unique_ptr<ExtendedGrammar::RhsNode> OneOf(Args&&... nodes) {
    return std::make_unique<ExtendedGrammar::RhsNodeChildren>(ExtendedGrammar::RhsNode::Type::OneOf, std::forward<Args>(nodes)...);
};

template<typename...Args> std::unique_ptr<ExtendedGrammar::RhsNode> Sequence(Args&&... nodes) {
    return std::make_unique<ExtendedGrammar::RhsNodeChildren>(ExtendedGrammar::RhsNode::Type::Sequence, std::forward<Args>(nodes)...);
};

DefReader::DefReader(const std::string &filename)
{
    std::vector<std::string> grammarTerminals{
        "epsilon",
        "terminal",
        "nonterminal",
        "colon",
        "pipe",
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
        return Tokenizer::InvalidTokenValue;
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
            pattern("'[^']+'", "literal"),
            pattern("\\s", "whitespace")  
        }},
        Tokenizer::Configuration{std::vector<Tokenizer::Pattern>{
            pattern("\\S+", "regex"),
            pattern("\\s", "whitespace")
        }}
    };

    Tokenizer tokenizer(std::move(configurations), tokenIndex("end"), tokenIndex("newline"));

    std::vector<ExtendedGrammar::Rule> grammarRules;
    grammarRules.push_back(ExtendedGrammar::Rule{"root"});
    grammarRules.push_back(ExtendedGrammar::Rule{"definitions"});
    grammarRules.push_back(ExtendedGrammar::Rule{"pattern"});
    grammarRules.push_back(ExtendedGrammar::Rule{"rule"});
    grammarRules.push_back(ExtendedGrammar::Rule{"rhs"});

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

    auto SetRule = [&](const std::string &name, std::unique_ptr<ExtendedGrammar::RhsNode> &&node) {
        grammarRules[ruleIndex(name)].rhs = std::move(node);
    };

    SetRule("root", Sequence(N("definitions"), T("end")));
    SetRule("definitions", ZeroOrMore(OneOf(N("pattern"), N("rule"), T("newline"))));
    SetRule("pattern", Sequence(T("terminal"), T("colon"), T("regex"), T("newline")));
    SetRule("rule", Sequence(T("nonterminal"), T("colon"), N("rhs"), ZeroOrMore(Sequence(T("pipe"), N("rhs"))), T("newline")));
    SetRule("rhs", OneOrMore(OneOf(T("terminal"), T("nonterminal"), T("literal"), T("epsilon"))));

    ExtendedGrammar extendedGrammar(std::move(grammarTerminals), std::move(grammarRules), 0);
    std::unique_ptr<Grammar> grammar = extendedGrammar.makeGrammar();
    LLParser parser(*grammar);
    
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
        std::unique_ptr<DefNode> list = std::make_unique<DefNode>(DefNode::Type::List);
        for(unsigned int i=2; i<numItems; i+=2) {
            list->children.push_back(std::move(items[i].data));
        }

        return std::make_unique<DefNode>(DefNode::Type::Rule, std::move(items[0].data), std::move(list));
    });
    session.addReducer("rhs", 0, [](LLParser::ParseItem<DefNode> *items, unsigned int numItems) {
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