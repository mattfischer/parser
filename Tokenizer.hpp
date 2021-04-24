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
    struct Configuration {
        Regex::Matcher matcher;
        unsigned int ignorePattern;
    };
    Tokenizer(std::vector<Configuration> &&configurations);
    Tokenizer(Regex::Matcher &&matcher, unsigned int ignorePattern);

    template<typename Data> struct Token {
        unsigned int index;
        unsigned int start;
        unsigned int line;
        std::unique_ptr<Data> data;
    };

    unsigned int endToken() const;
    unsigned int errorToken() const;

    template<typename TokenData> class Stream
    {
    public:
        typedef std::function<std::unique_ptr<TokenData>(unsigned int, const std::string&)> Decorator;
        Stream(const Tokenizer &tokenizer, std::istream &input, Decorator decorator)
        : mTokenizer(tokenizer), mInput(input), mDecorator(decorator)
        {
            mConsumed = 0;
            mLine = 0;
            mConfiguration = 0;

            consumeToken();
        }

        void setConfiguration(unsigned int configuration)
        {
            mConfiguration = configuration;
        }

        unsigned int configuration() const
        {
            return mConfiguration;
        }

        const Token<TokenData> &nextToken() const
        {
            return mNextToken;
        }

        void consumeToken()
        {
            if(mNextToken.index == mTokenizer.mErrorToken || mNextToken.index == mTokenizer.mEndToken) {
                return;
            }

            bool repeat = true;
            while(repeat) {
                while(mConsumed >= mCurrentLine.size()) {
                    if(mInput.eof() || mInput.fail()) {
                        mNextToken.index = mTokenizer.mEndToken;
                        mNextToken.start = mConsumed;
                        mNextToken.line = mLine;
                        return;
                    }

                    std::getline(mInput, mCurrentLine);
                    mConsumed = 0;
                    mLine++;
                }

                unsigned int pattern;
                unsigned int matched = mTokenizer.mConfigurations[mConfiguration].matcher.match(mCurrentLine, mConsumed, pattern);
                if(matched == 0) {
                    mNextToken.index = mTokenizer.mErrorToken;
                    mNextToken.start = mConsumed;
                    mNextToken.line = mLine;

                    repeat = false;
                } else {
                    if(pattern != mTokenizer.mConfigurations[mConfiguration].ignorePattern) {
                        mNextToken.index = pattern;
                        mNextToken.start = mConsumed;
                        mNextToken.line = mLine;
                        if(mDecorator) {
                            std::string text = mCurrentLine.substr(mConsumed, matched);
                            mNextToken.data = mDecorator(pattern, text);
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
        Decorator mDecorator;
    };

private:
    void initialize();

    std::vector<Configuration> mConfigurations;

    unsigned int mEndToken;
    unsigned int mErrorToken;
    unsigned int mIgnorePattern;
};

#endif