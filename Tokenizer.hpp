#ifndef TOKENIZER_HPP
#define TOKENIZER_HPP

#include "Regex/Matcher.hpp"

#include <vector>
#include <string>

class Tokenizer
{
public:
    Tokenizer(const Regex::Matcher &matcher, unsigned int ignorePattern, const std::string &input);

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
    const Regex::Matcher &mMatcher;
    const std::string &mInput;

    unsigned int mConsumed;
    unsigned int mEndToken;
    unsigned int mErrorToken;
    unsigned int mIgnorePattern;
    Token mNextToken;
};

#endif