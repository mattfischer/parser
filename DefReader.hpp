#ifndef DEFREADER_HPP
#define DEFREADER_HPP

#include "Regex/Matcher.hpp"
#include "Parser.hpp"

#include <string>
#include <vector>
#include <map>
#include <memory>

class DefReader {
public:
    DefReader(const std::string &filename);

    bool valid() const;

    const Regex::Matcher &matcher() const;
    const Parser &parser() const;
    unsigned int ignorePattern() const;

    struct ParseError {
        unsigned int line;
        std::string message;
    };
    const ParseError &parseError() const;

private:
    typedef std::vector<std::vector<std::string>> Rule;
    bool parseFile(const std::string &filename, std::map<std::string, std::string> &terminals, std::map<std::string, Rule> &rules);

    std::unique_ptr<Regex::Matcher> mMatcher;
    std::vector<Parser::Rule> mParserRules;
    std::unique_ptr<Parser> mParser;
    unsigned int mIgnorePattern;
    ParseError mParseError;
};
#endif