#ifndef TOKENIZER_HPP
#define TOKENIZER_HPP

#include "Regex/Matcher.hpp"

#include <vector>
#include <string>

class Tokenizer
{
public:
    Tokenizer(const Regex::Matcher &matcher, unsigned int ignorePattern);

    struct Token {
        unsigned int index;
        unsigned int start;
        unsigned int length;
    };

    unsigned int endToken() const;
    unsigned int errorToken() const;

    class Stream
    {
    public:
        Stream(const Tokenizer &tokenizer, const std::string &input);

        const Token &nextToken() const;
        void consumeToken();

    private:
        const Tokenizer &mTokenizer;
        const std::string &mInput;
        unsigned int mConsumed;
        Token mNextToken;
    };

private:
    const Regex::Matcher &mMatcher;

    unsigned int mEndToken;
    unsigned int mErrorToken;
    unsigned int mIgnorePattern;
};

#endif