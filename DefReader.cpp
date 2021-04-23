#include "DefReader.hpp"

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

    unsigned int ignorePattern = UINT_MAX;
    std::map<std::string, unsigned int> terminalMap;
    std::map<std::string, unsigned int> anonymousTerminalMap;
    std::vector<std::string> terminals;
    for(const auto &pair: terminalDef) {
        terminals.push_back(pair.second);
        terminalMap[pair.first] = (unsigned int)(terminals.size() - 1);
        if(pair.first == "IGNORE") {
            ignorePattern = (unsigned int)(terminals.size() - 1);
        }
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

    auto it = ruleMap.find("<root>");
    if(it == ruleMap.end()) {
        mParseError.message = "No <root> nonterminal defined";
        return;
    } else {
        Grammar::Symbol endSymbol;
        endSymbol.type = Grammar::Symbol::Type::Terminal;
        endSymbol.index = (unsigned int)terminals.size();
        endSymbol.name = "#";
        for(auto &rhs: rules[it->second].rhs) {
            rhs.push_back(endSymbol);
        }

        Regex::Matcher matcher(terminals);
        mTokenizer = std::make_unique<Tokenizer>(std::move(matcher), ignorePattern);
        mGrammar = std::make_unique<Grammar>(std::move(rules), it->second);
    }
}

void DefReader::expectToken(Tokenizer::Stream &stream, unsigned int token, const std::string &text)
{
    if(stream.nextToken().index == token) {
        stream.consumeToken();
    } else {
        mParseError.line = stream.nextToken().line;
        mParseError.message = "Expected " + text + " , found " + stream.nextToken().text;
        throw std::exception();
    }
}

bool DefReader::parseFile(const std::string &filename, std::map<std::string, std::string> &terminals, std::map<std::string, Rule> &rules)
{
    std::vector<Tokenizer::Configuration> configurations;
    
    enum PrimaryToken {
        Terminal,
        Nonterminal,
        Colon,
        Pipe,
        Literal,
        Whitespace
    };
    std::vector<std::string> primaryPatterns{"\\w+", "<\\w+>", ":", "\\|", "'[^']+'", "\\s" };
    configurations.push_back(Tokenizer::Configuration{primaryPatterns, Whitespace});

    enum RegexToken {
        Regex,
        RegexWhitespace
    };
    std::vector<std::string> regexPatterns{"\\S+", "\\s"};
    configurations.push_back(Tokenizer::Configuration{regexPatterns, RegexWhitespace});

    Tokenizer tokenizer(std::move(configurations));

    std::ifstream file(filename);
    Tokenizer::Stream stream(tokenizer, file);

    try {
        while(stream.nextToken().index != tokenizer.endToken()) {
            if(stream.nextToken().index == Terminal) {
                std::string name = stream.nextToken().text;
                stream.consumeToken();

                stream.setConfiguration(1);
                expectToken(stream, Colon, ":");

                std::string pattern = stream.nextToken().text;
                stream.setConfiguration(0);
                stream.consumeToken();
                terminals[name] = pattern;
            } else if(stream.nextToken().index == Nonterminal) {
                std::string name = stream.nextToken().text;
                stream.consumeToken();

                expectToken(stream, Colon, ":");
                unsigned int line = stream.nextToken().line;
                std::vector<std::string> symbols;
                while(stream.nextToken().line == line) {
                    if(stream.nextToken().index == PrimaryToken::Pipe) {
                        rules[name].push_back(symbols);
                        symbols.clear();
                    } else {
                        symbols.push_back(stream.nextToken().text);
                    }
                    stream.consumeToken();
                }
                rules[name].push_back(symbols);
            } else {
                mParseError.line = stream.nextToken().line;
                mParseError.message = "Unexpected token " + stream.nextToken().text;
                return false;
            }
        }
    } catch(std::exception e) {
        return false;
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