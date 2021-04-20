#include "DefReader.hpp"

#include <fstream>
#include <vector>
#include <iostream>

std::string trim(const std::string &input)
{
    size_t start = input.find_first_not_of(" ");
    if(start == std::string::npos) {
        return "";
    }
    size_t end = input.find_last_not_of(" ");
    return input.substr(start, end-start+1);
}

std::vector<std::string> split(const std::string &input, char c)
{
    std::vector<std::string> results;

    size_t start = 0;
    while(start < input.size()) {
        size_t end = input.find(c, start);
        if(end == std::string::npos) {
            end = input.size();
        }

        std::string substr = input.substr(start, end-start);
        if(substr.size() > 0) {
            results.push_back(substr);
        }

        start = end + 1;
    }

    return results;
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
        terminalMap[pair.first] = terminals.size() - 1;
        if(pair.first == "IGNORE") {
            ignorePattern = terminals.size() - 1;
        }
    }

    std::map<std::string, unsigned int> ruleMap;
    for(const auto &pair: ruleDef) {
        mParserRules.push_back(Parser::Rule());
        ruleMap[pair.first] = mParserRules.size() - 1;
    }

    for(const auto &pair: ruleDef) {
        Parser::Rule &rule = mParserRules[ruleMap[pair.first]];
        for(const auto &r : pair.second) {
            Parser::RHS rhs;
            for(const auto &s : r) {
                Parser::Symbol symbol;
                if(s[0] == '<') {
                    symbol.type = Parser::Symbol::Type::Nonterminal;
                    auto it = ruleMap.find(s);
                    if(it == ruleMap.end()) {
                        mParseError.message = "Unknown nonterminal " + s;
                        return;
                    } else {
                        symbol.index = it->second;
                    }
                } else if(s[0] == '\'') {
                    std::string text = s.substr(1, s.size() - 2);
                    text = escape(text);
                    symbol.type = Parser::Symbol::Type::Terminal;

                    auto it = anonymousTerminalMap.find(text);
                    if(it == anonymousTerminalMap.end()) {
                        terminals.push_back(text);
                        symbol.index = anonymousTerminalMap[text] = terminals.size() - 1;
                    } else {
                        symbol.index = it->second;
                    }
                } else {
                    symbol.type = Parser::Symbol::Type::Terminal;
                    auto it = terminalMap.find(s);
                    if(it == terminalMap.end()) {
                        mParseError.message = "Unknown terminal " + s;
                        return;
                    } else {
                        symbol.index = it->second;
                    }
                }
                rhs.symbols.push_back(symbol);
            }
            rule.rhs.push_back(rhs);
        }
    }

    auto it = ruleMap.find("<root>");
    if(it == ruleMap.end()) {
        mParseError.message = "No <root> nonterminal defined";
        return;
    } else {
        mMatcher = std::make_unique<Regex::Matcher>(terminals);
        mParser = std::make_unique<Parser>(mParserRules, it->second, ignorePattern);
    }
}

bool DefReader::parseFile(const std::string &filename, std::map<std::string, std::string> &terminals, std::map<std::string, Rule> &rules)
{
    std::ifstream file(filename);
    unsigned int lineNumber = 0;

    while(!file.fail() && !file.eof()) {
        lineNumber++;
        std::string line;
        std::getline(file, line);
        line = trim(line);
        if(line.size() == 0) {
            continue;
        }

        size_t pos = line.find(':');
        if(pos == std::string::npos) {
            mParseError.line = lineNumber;
            mParseError.message = "Invalid line";
            return false;
        }    

        std::string left = trim(line.substr(0, pos));
        std::string right = trim(line.substr(pos+1));
        if(left[0] == '<') {
            if(left[left.size()-1] != '>') {
                mParseError.line = lineNumber;
                mParseError.message = "Invalid symbol name " + left;
                return false;
            }
            std::vector<std::string> rhs = split(right, '|');
            for(const std::string &r : rhs) {
                std::vector<std::string> symbols = split(r, ' ');
                rules[left].push_back(symbols);
            }
        } else {
            terminals[left] = right;
        }
    }

    return true;
}

bool DefReader::valid() const
{
    return mMatcher && mParser;
}

const Regex::Matcher &DefReader::matcher() const
{
    return *mMatcher;
}

const DefReader::ParseError &DefReader::parseError() const
{
    return mParseError;
}

const Parser &DefReader::parser() const
{
    return *mParser;
}
