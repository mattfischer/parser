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

DefReader::DefReader(const std::string &filename)
{
    std::map<std::string, std::string> terminalDef;
    std::map<std::string, Rule> ruleDef;
    parseFile(filename, terminalDef, ruleDef);

    std::map<std::string, unsigned int> terminalMap;
    std::vector<std::string> terminals;
    for(const auto &pair: terminalDef) {
        terminals.push_back(pair.second);
        terminalMap[pair.first] = terminals.size() - 1;
    }
    mMatcher = std::make_unique<Regex::Matcher>(terminals);

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
                    symbol.index = ruleMap[s];
                } else {
                    symbol.type = Parser::Symbol::Type::Terminal;
                    symbol.index = terminalMap[s];
                }
                rhs.symbols.push_back(symbol);
            }
            rule.rhs.push_back(rhs);
        }
    }

    mParser = std::make_unique<Parser>(mParserRules, ruleMap["<root>"]);
}

void DefReader::parseFile(const std::string &filename, std::map<std::string, std::string> &terminals, std::map<std::string, Rule> &rules)
{
    std::ifstream file(filename);
    
    while(!file.fail() && !file.eof()) {
        std::string line;
        std::getline(file, line);
        if(line.size() == 0) {
            continue;
        }

        size_t pos = line.find(':');
        if(pos == std::string::npos) {
            continue;
        }    

        std::string left = trim(line.substr(0, pos));
        std::string right = trim(line.substr(pos+1));
        if(left[0] == '<') {
            std::vector<std::string> rhs = split(right, '|');
            for(const std::string &r : rhs) {
                std::vector<std::string> symbols = split(r, ' ');
                rules[left].push_back(symbols);
            }
        } else {
            terminals[left] = right;
        }
    }
}

bool DefReader::valid() const
{
    return mMatcher && mParser;
}

const Regex::Matcher &DefReader::matcher() const
{
    return *mMatcher;
}

const Parser &DefReader::parser() const
{
    return *mParser;
}
