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
        Regex,
        Pattern,
        Rule,
        RhsSequence,
        RhsOneOf,
        RhsZeroOrMore,
        RhsOneOrMore,
        RhsZeroOrOne
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

std::unique_ptr<ExtendedGrammar::RhsNode> createRhsNode(
    const DefNode &defNode,
    std::map<std::string, unsigned int> &ruleMap,
    std::map<std::string, unsigned int> &terminalMap,
    std::map<std::string, unsigned int> &anonymousTerminalMap,
    std::vector<std::string> &terminals,
    std::vector<std::string> &terminalNames)
{
    switch(defNode.type) {
        case DefNode::Type::Terminal:
        {
            unsigned int index = UINT_MAX;
            auto it = terminalMap.find(defNode.string);
            if(it != terminalMap.end()) {
                index = it->second;
            }
            return std::make_unique<ExtendedGrammar::RhsNodeSymbol>(ExtendedGrammar::RhsNodeSymbol::SymbolType::Terminal, index);
        }

        case DefNode::Type::Nonterminal:
        {
            unsigned int index = UINT_MAX;
            auto it = ruleMap.find(defNode.string);
            if(it != ruleMap.end()) {
                index = it->second;
            }
            return std::make_unique<ExtendedGrammar::RhsNodeSymbol>(ExtendedGrammar::RhsNodeSymbol::SymbolType::Nonterminal, index);
        }

        case DefNode::Type::Literal:
        {
            std::string text = escape(defNode.string);
            unsigned int index;

            auto it = anonymousTerminalMap.find(text);
            if(it == anonymousTerminalMap.end()) {
                terminals.push_back(text);
                terminalNames.push_back(text);
                index = anonymousTerminalMap[text] = (unsigned int)(terminals.size() - 1);
            } else {
                index = it->second;
            }
            return std::make_unique<ExtendedGrammar::RhsNodeSymbol>(ExtendedGrammar::RhsNodeSymbol::SymbolType::Terminal, index);
        }

        case DefNode::Type::RhsSequence:
        {
            std::unique_ptr<ExtendedGrammar::RhsNodeChildren> node = std::make_unique<ExtendedGrammar::RhsNodeChildren>(ExtendedGrammar::RhsNode::Type::Sequence);
            for(auto &child : defNode.children) {
                std::unique_ptr<ExtendedGrammar::RhsNode> childRhsNode = createRhsNode(*child, ruleMap, terminalMap, anonymousTerminalMap, terminals, terminalNames);
                node->children.push_back(std::move(childRhsNode));
            }
            return node;
        }

        case DefNode::Type::RhsOneOf:
        {
            std::unique_ptr<ExtendedGrammar::RhsNodeChildren> node = std::make_unique<ExtendedGrammar::RhsNodeChildren>(ExtendedGrammar::RhsNode::Type::OneOf);
            for(auto &child : defNode.children) {
                std::unique_ptr<ExtendedGrammar::RhsNode> childRhsNode = createRhsNode(*child, ruleMap, terminalMap, anonymousTerminalMap, terminals, terminalNames);
                node->children.push_back(std::move(childRhsNode));
            }
            return node;
        }

        case DefNode::Type::RhsZeroOrMore:
        {
            std::unique_ptr<ExtendedGrammar::RhsNode> child = createRhsNode(*defNode.children[0], ruleMap, terminalMap, anonymousTerminalMap, terminals, terminalNames);
            return std::make_unique<ExtendedGrammar::RhsNodeChild>(ExtendedGrammar::RhsNode::Type::ZeroOrMore, std::move(child));
        }

        case DefNode::Type::RhsOneOrMore:
        {
            std::unique_ptr<ExtendedGrammar::RhsNode> child = createRhsNode(*defNode.children[0], ruleMap, terminalMap, anonymousTerminalMap, terminals, terminalNames);
            return std::make_unique<ExtendedGrammar::RhsNodeChild>(ExtendedGrammar::RhsNode::Type::OneOrMore, std::move(child));
        }

        case DefNode::Type::RhsZeroOrOne:
        {
            std::unique_ptr<ExtendedGrammar::RhsNode> child = createRhsNode(*defNode.children[0], ruleMap, terminalMap, anonymousTerminalMap, terminals, terminalNames);
            return std::make_unique<ExtendedGrammar::RhsNodeChild>(ExtendedGrammar::RhsNode::Type::ZeroOrOne, std::move(child));
        }
    }
    return std::unique_ptr<ExtendedGrammar::RhsNode>();
};

DefReader::DefReader(const std::string &filename)
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

    Tokenizer tokenizer(std::move(configurations), tokenIndex("end"), tokenIndex("newline"));

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

    auto SetRule = [&](const std::string &name, std::unique_ptr<ExtendedGrammar::RhsNode> &&node) {
        grammarRules[ruleIndex(name)].rhs = std::move(node);
    };

    SetRule("root", Sequence(N("definitions"), T("end")));
    SetRule("definitions", ZeroOrMore(OneOf(N("pattern"), N("rule"), T("newline"))));
    SetRule("pattern", Sequence(T("terminal"), T("colon"), T("regex"), T("newline")));
    SetRule("rule", Sequence(T("nonterminal"), T("colon"), N("rhs"), T("newline")));
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
    session.addMatchListener("pattern", [&](unsigned int symbol) {
        if(symbol == 1) stream.setConfiguration(1);
        else if(symbol == 2) stream.setConfiguration(0);
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

    session.addReducer("root", [](LLParser::ParseItem<DefNode> *items, unsigned int numItems) {
        return std::move(items[0].data);
    });
    session.addReducer("definitions", [](LLParser::ParseItem<DefNode> *items, unsigned int numItems) {
        std::unique_ptr<DefNode> node = std::make_unique<DefNode>(DefNode::Type::List);
        for(unsigned int i=0; i<numItems; i++) {
            if(items[i].data) {
                node->children.push_back(std::move(items[i].data));
            }
        }
        return node;
    });
    session.addReducer("pattern", [](LLParser::ParseItem<DefNode> *items, unsigned int numItems) {
        return std::make_unique<DefNode>(DefNode::Type::Pattern, std::move(items[0].data), std::move(items[2].data));
    });
    session.addReducer("rule", [](LLParser::ParseItem<DefNode> *items, unsigned int numItems) {
        return std::make_unique<DefNode>(DefNode::Type::Rule, std::move(items[0].data), std::move(items[2].data));
    });
    session.addReducer("rhs", [](LLParser::ParseItem<DefNode> *items, unsigned int numItems) {
        if(numItems == 1) {
            return std::move(items[0].data);
        } else {
            std::unique_ptr<DefNode> node = std::make_unique<DefNode>(DefNode::Type::RhsSequence);
            for(unsigned int i=0; i<numItems; i++) {
                node->children.push_back(std::move(items[i].data));
            }
            return node;
        }
    });
    unsigned int star = grammar->terminalIndex("star");
    unsigned int plus = grammar->terminalIndex("plus");
    unsigned int question = grammar->terminalIndex("question");
    session.addReducer("rhsSuffix", [&](LLParser::ParseItem<DefNode> *items, unsigned int numItems) {
        std::unique_ptr<DefNode> node = std::move(items[0].data);
        for(unsigned int i=1; i<numItems; i++) {
            if(items[i].index == star) {
                node = std::make_unique<DefNode>(DefNode::Type::RhsZeroOrMore, std::move(node));
            } else if(items[i].index == plus) {
                node = std::make_unique<DefNode>(DefNode::Type::RhsOneOrMore, std::move(node));
            } else if(items[i].index == question) {
                node = std::make_unique<DefNode>(DefNode::Type::RhsZeroOrOne, std::move(node));
            }
        }
        return node;
    });
    unsigned int lparen = grammar->terminalIndex("lparen");
    session.addReducer("rhsSymbol", [&](LLParser::ParseItem<DefNode> *items, unsigned int numItems) {
        if(numItems == 1) {
            return std::move(items[0].data);        
        } else if(numItems == 3) {
            return std::move(items[1].data);
        } else {
            std::unique_ptr<DefNode> node = std::make_unique<DefNode>(DefNode::Type::RhsOneOf);
            for(unsigned int i=1; i<numItems; i+=2) {
                node->children.push_back(std::move(items[i].data));
            }
            return node;
        }

        return std::unique_ptr<DefNode>();
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

    std::vector<ExtendedGrammar::Rule> rules;
    std::map<std::string, unsigned int> ruleMap;
    for(const auto &definition: node->children) {
        if(definition->type == DefNode::Type::Rule) {
            rules.push_back(ExtendedGrammar::Rule());
            ruleMap[definition->children[0]->string] = (unsigned int)(rules.size() - 1);
        }
    }

    for(const auto &definition: node->children) {
        if(definition->type == DefNode::Type::Rule) {
            std::string name = definition->children[0]->string;
            ExtendedGrammar::Rule &rule = rules[ruleMap[name]];
            rule.lhs = name;
            rule.rhs = createRhsNode(*definition->children[1], ruleMap, terminalMap, anonymousTerminalMap, terminals, terminalNames);
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
        ExtendedGrammar::Rule &rule = rules[it->second];
        std::unique_ptr<ExtendedGrammar::RhsNode> endNode = std::make_unique<ExtendedGrammar::RhsNodeSymbol>(ExtendedGrammar::RhsNodeSymbol::SymbolType::Terminal, endValue);
        if(rule.rhs->type == ExtendedGrammar::RhsNode::Type::Sequence) {
            static_cast<ExtendedGrammar::RhsNodeChildren&>(*rule.rhs).children.push_back(std::move(endNode));
        } else {
            std::unique_ptr<ExtendedGrammar::RhsNode> newNode = std::make_unique<ExtendedGrammar::RhsNodeChildren>(ExtendedGrammar::RhsNode::Type::Sequence, std::move(rule.rhs), std::move(endNode));
            rule.rhs = std::move(newNode);
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
        ExtendedGrammar extendedGrammar(std::move(terminalNames), std::move(rules), it->second);
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

const Grammar &DefReader::grammar() const
{
    return *mGrammar;
}