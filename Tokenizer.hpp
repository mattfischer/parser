#ifndef TOKENIZER_HPP
#define TOKENIZER_HPP

#include "Regex/Matcher.hpp"

#include <vector>
#include <string>
#include <istream>

class Tokenizer
{
public:
    struct Configuration {
        Regex::Matcher matcher;
        unsigned int ignorePattern;
    };
    Tokenizer(std::vector<Configuration> &&configurations);
    Tokenizer(Regex::Matcher &&matcher, unsigned int ignorePattern);

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

        void setConfiguration(unsigned int configuration);
        unsigned int configuration() const;

    private:
        const Tokenizer &mTokenizer;
        std::istream &mInput;
        std::string mCurrentLine;
        unsigned int mConsumed;
        Token mNextToken;
        unsigned int mLine;
        unsigned int mConfiguration;
    };

private:
    void initialize();

    std::vector<Configuration> mConfigurations;

    unsigned int mEndToken;
    unsigned int mErrorToken;
    unsigned int mIgnorePattern;
};

#endif