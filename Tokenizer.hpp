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
    Tokenizer(std::vector<Configuration> &&configurations, TokenValue endValue, TokenValue newlineValue);

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
        Stream(const Tokenizer &tokenizer, std::istream &input)
        : mTokenizer(tokenizer), mInput(input)
        {
            mConsumed = 0;
            mLine = 0;
            mConfiguration = 0;
            mNextToken = {kInvalidTokenValue, 0, 0};
        }

        void setConfiguration(unsigned int configuration)
        {
            if(configuration < mTokenizer.mConfigurations.size()) {
                mConfiguration = configuration;
            }
        }

        unsigned int configuration() const
        {
            return mConfiguration;
        }

        const Token &nextToken()
        {
            if(mLine == 0) {
                consumeToken();
            }
            return mNextToken;
        }

        void consumeToken()
        {
            if(mNextToken.value == kErrorTokenValue || mNextToken.value == mTokenizer.mEndValue) {
                return;
            }

            bool repeat = true;
            while(repeat) {
                while(mConsumed >= mCurrentLine.size()) {
                    if(mConsumed == mCurrentLine.size() && mTokenizer.mNewlineValue != kInvalidTokenValue && mLine > 0) {
                        mNextToken.value = mTokenizer.mNewlineValue;
                        mNextToken.start = mConsumed;
                        mNextToken.line = mLine;
                        mNextToken.text = "<newline>";
                        mConsumed++;
                        return;
                    }

                    if(mInput.eof() || mInput.fail()) {
                        mNextToken.value = mTokenizer.mEndValue;
                        mNextToken.start = mConsumed;
                        mNextToken.line = mLine;
                        mNextToken.text = "<end>";
                        return;
                    }

                    std::getline(mInput, mCurrentLine);
                    mConsumed = 0;
                    mLine++;
                }

                unsigned int pattern;
                unsigned int matched = mTokenizer.mMatchers[mConfiguration]->match(mCurrentLine, mConsumed, pattern);
                if(matched == 0) {
                    mNextToken.value = kErrorTokenValue;
                    mNextToken.start = mConsumed;
                    mNextToken.line = mLine;
                    mNextToken.text = mCurrentLine.substr(mConsumed, 1);

                    repeat = false;
                } else {
                    TokenValue value = mTokenizer.mConfigurations[mConfiguration].patterns[pattern].value;
                    if(value != kInvalidTokenValue) {
                        mNextToken.value = value;
                        mNextToken.start = mConsumed;
                        mNextToken.line = mLine;
                        mNextToken.text = mCurrentLine.substr(mConsumed, matched);

                        repeat = false;
                    }

                    mConsumed += matched;
                }
            }
        }

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