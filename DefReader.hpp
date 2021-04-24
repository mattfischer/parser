#ifndef DEFREADER_HPP
#define DEFREADER_HPP

#include "Regex/Matcher.hpp"
#include "Tokenizer.hpp"
#include "Grammar.hpp"

#include <string>
#include <vector>
#include <map>
#include <memory>

class DefReader {
public:
    DefReader(const std::string &filename);

    bool valid() const;

    const Tokenizer &tokenizer() const;
    const Grammar &grammar() const;

    struct ParseError {
        unsigned int line;
        std::string message;
    };
    const ParseError &parseError() const;

private:
    typedef std::vector<std::vector<std::string>> Rule;
    bool parseFile(const std::string &filename, std::map<std::string, std::string> &terminals, std::map<std::string, Rule> &rules);

    std::unique_ptr<Tokenizer> mTokenizer;
    std::unique_ptr<Grammar> mGrammar;
    ParseError mParseError;
};
#endif