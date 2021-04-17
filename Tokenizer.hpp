#ifndef TOKENIZER_HPP
#define TOKENIZER_HPP

#include "Regex/Matcher.hpp"

#include <vector>
#include <string>

class Tokenizer
{
public:
    Tokenizer(const std::vector<std::string> &patterns, const std::string &input);

    bool valid() const;
    const Regex::Matcher::ParseError &regexParseError() const;

    struct Token {
        unsigned int index;
        unsigned int start;
        unsigned int length;
    };
    const Token &nextToken() const;
    void consumeToken();

    unsigned int endToken() const;
    unsigned int errorToken() const;

private:
    Regex::Matcher mMatcher;
    const std::string &mInput;
    unsigned int mConsumed;
    unsigned int mEndToken;
    unsigned int mErrorToken;
    Token mNextToken;
};

#endif