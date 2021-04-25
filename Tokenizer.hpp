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
        std::vector<std::string> patternNames;
    };
    Tokenizer(std::vector<Configuration> &&configurations);
    Tokenizer(Regex::Matcher &&matcher, unsigned int ignorePattern, std::vector<std::string> &&patternNames);

    template<typename Data> struct Token {
        unsigned int index;
        unsigned int start;
        unsigned int line;
        std::unique_ptr<Data> data;
    };

    unsigned int endToken() const;
    unsigned int errorToken() const;

    unsigned int patternIndex(const std::string &name, unsigned int configuration) const;

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
            mStarted = false;
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
            unsigned int patternIndex = mTokenizer.patternIndex(pattern, configuration);
            if(patternIndex != UINT_MAX) {
                mDecorators[std::pair<unsigned int, unsigned int>(patternIndex, configuration)] = decorator;
            }
        }

        const Token<TokenData> &nextToken()
        {
            if(!mStarted) {
                consumeToken();
                mStarted = true;
            }
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
                    mCurrentLine += '\n';
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
                        auto it = mDecorators.find(std::pair<unsigned int, unsigned int>(pattern, mConfiguration));
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
        std::map<std::pair<unsigned int, unsigned int>, Decorator> mDecorators;
        bool mStarted;
    };

private:
    void initialize();

    std::vector<Configuration> mConfigurations;

    unsigned int mEndToken;
    unsigned int mErrorToken;
    unsigned int mIgnorePattern;
};

#endif