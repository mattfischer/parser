#include "Tokenizer.hpp"

#include <algorithm>

Tokenizer::Tokenizer(std::vector<Configuration> &&configurations)
: mConfigurations(std::move(configurations))
{
    initialize();
}

Tokenizer::Tokenizer(Regex::Matcher &&matcher, unsigned int ignorePattern)
{
    mConfigurations.push_back(Configuration{std::move(matcher), ignorePattern});
    initialize();
}

void Tokenizer::initialize()
{
    mEndToken = 0;
    for(const auto &configuration : mConfigurations) {
        mEndToken = std::max(mEndToken, configuration.matcher.numPatterns());
    }
    mErrorToken = mEndToken+1;
}

unsigned int Tokenizer::endToken() const
{
    return mEndToken;
}

unsigned int Tokenizer::errorToken() const
{
    return mErrorToken;
}

Tokenizer::Stream::Stream(const Tokenizer &tokenizer, std::istream &input, Decorator decorator)
: mTokenizer(tokenizer), mInput(input), mDecorator(decorator)
{
    mConsumed = 0;
    mLine = 0;
    mConfiguration = 0;

    consumeToken();
}

void Tokenizer::Stream::setConfiguration(unsigned int configuration)
{
    mConfiguration = configuration;
}

unsigned int Tokenizer::Stream::configuration() const
{
    return mConfiguration;
}

const Tokenizer::Token &Tokenizer::Stream::nextToken() const
{
    return mNextToken;
}

void Tokenizer::Stream::consumeToken()
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
                    mDecorator(mNextToken, text);
                }
                repeat = false;
            }

            mConsumed += matched;
        }
    }
}
