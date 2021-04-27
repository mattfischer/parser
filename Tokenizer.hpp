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
    static const TokenValue InvalidTokenValue = UINT_MAX;
    static const TokenValue ErrorTokenValue = InvalidTokenValue -1;

    struct Pattern {
        std::string regex;
        std::string name;
        TokenValue value;
    };

    struct Configuration {
        std::vector<Pattern> patterns;
    };
    Tokenizer(std::vector<Configuration> &&configurations, TokenValue endValue, TokenValue newlineValue);

    template<typename Data> struct Token {
        TokenValue value;
        unsigned int start;
        unsigned int line;
        std::unique_ptr<Data> data;
    };

    unsigned int patternValue(const std::string &name, unsigned int configuration) const;

    template<typename TokenData> class Stream
    {
    public:
        typedef std::function<std::unique_ptr<TokenData>(const std::string&)> Decorator;
        Stream(const Tokenizer &tokenizer, std::istream &input)
        : mTokenizer(tokenizer), mInput(input)
        {
            mConsumed = 0;
            mLine = 0;
            mConfiguration = 0;
        }

        void setConfiguration(unsigned int configuration)
        {
            mConfiguration = configuration;
        }

        unsigned int configuration() const
        {
            return mConfiguration;
        }

        void addDecorator(const std::string &pattern, unsigned int configuration, Decorator decorator)
        {
            TokenValue patternValue = mTokenizer.patternValue(pattern, configuration);
            if(patternValue != InvalidTokenValue) {
                mDecorators[patternValue] = decorator;
            }
        }

        const Token<TokenData> &nextToken()
        {
            if(mLine == 0) {
                consumeToken();
            }
            return mNextToken;
        }

        void consumeToken()
        {
            if(mNextToken.value == ErrorTokenValue || mNextToken.value == mTokenizer.mEndValue) {
                return;
            }

            bool repeat = true;
            while(repeat) {
                while(mConsumed >= mCurrentLine.size()) {
                    if(mConsumed == mCurrentLine.size() && mTokenizer.mNewlineValue != InvalidTokenValue && mLine > 0) {
                        mNextToken.value = mTokenizer.mNewlineValue;
                        mNextToken.start = mConsumed;
                        mNextToken.line = mLine;
                        mConsumed++;
                        return;
                    }

                    if(mInput.eof() || mInput.fail()) {
                        mNextToken.value = mTokenizer.mEndValue;
                        mNextToken.start = mConsumed;
                        mNextToken.line = mLine;
                        return;
                    }

                    std::getline(mInput, mCurrentLine);
                    mConsumed = 0;
                    mLine++;
                }

                unsigned int pattern;
                unsigned int matched = mTokenizer.mMatchers[mConfiguration]->match(mCurrentLine, mConsumed, pattern);
                if(matched == 0) {
                    mNextToken.value = ErrorTokenValue;
                    mNextToken.start = mConsumed;
                    mNextToken.line = mLine;

                    repeat = false;
                } else {
                    TokenValue value = mTokenizer.mConfigurations[mConfiguration].patterns[pattern].value;
                    if(value != InvalidTokenValue) {
                        mNextToken.value = value;
                        mNextToken.start = mConsumed;
                        mNextToken.line = mLine;
                        auto it = mDecorators.find(value);
                        if(it != mDecorators.end()) {
                            std::string text = mCurrentLine.substr(mConsumed, matched);
                            mNextToken.data = it->second(text);
                        }
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
        Token<TokenData> mNextToken;
        unsigned int mLine;
        unsigned int mConfiguration;
        std::map<TokenValue, Decorator> mDecorators;
    };

private:
    void initialize();

    std::vector<Configuration> mConfigurations;
    std::vector<std::unique_ptr<Regex::Matcher>> mMatchers;

    TokenValue mEndValue;
    TokenValue mNewlineValue;
};

#endif