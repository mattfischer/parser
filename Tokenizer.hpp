#ifndef TOKENIZER_HPP
#define TOKENIZER_HPP

#include "Regex/Matcher.hpp"

#include <vector>
#include <string>
#include <istream>
#include <functional>

class Tokenizer
{
public:
    typedef unsigned int TokenValue;
    static const TokenValue kInvalidTokenValue = UINT_MAX;
    static const TokenValue kErrorTokenValue = kInvalidTokenValue -1;

    struct Pattern {
        std::string regex;
        std::string name;
        TokenValue value;
    };

    struct Configuration {
        std::vector<Pattern> patterns;
    };
    Tokenizer(std::vector<Configuration> configurations, TokenValue endValue, TokenValue newlineValue);

    struct Token {
        TokenValue value;
        unsigned int start;
        unsigned int line;
        std::string text;
    };

    unsigned int patternValue(const std::string &name, unsigned int configuration) const;

    class Stream
    {
    public:
        Stream(const Tokenizer &tokenizer, std::istream &input);

        void setConfiguration(unsigned int configuration);
        unsigned int configuration() const;

        const Token &nextToken();
        void consumeToken();

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
    std::vector<std::unique_ptr<Regex::Matcher>> mMatchers;

    TokenValue mEndValue;
    TokenValue mNewlineValue;
};

#endif