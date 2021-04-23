#ifndef TOKENIZER_HPP
#define TOKENIZER_HPP

#include "Regex/Matcher.hpp"

#include <vector>
#include <string>
#include <istream>

class Tokenizer
{
public:
    Tokenizer(const Regex::Matcher &matcher, unsigned int ignorePattern);

    struct Token {
        unsigned int index;
        unsigned int start;
        unsigned int line;
        std::string text;
    };

    unsigned int endToken() const;
    unsigned int errorToken() const;

    class Stream
    {
    public:
        Stream(const Tokenizer &tokenizer, std::istream &input);

        const Token &nextToken() const;
        void consumeToken();

    private:
        const Tokenizer &mTokenizer;
        std::istream &mInput;
        std::string mCurrentLine;
        unsigned int mConsumed;
        Token mNextToken;
        unsigned int mLine;
    };

private:
    const Regex::Matcher &mMatcher;

    unsigned int mEndToken;
    unsigned int mErrorToken;
    unsigned int mIgnorePattern;
};

#endif