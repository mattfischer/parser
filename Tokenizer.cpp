#include "Tokenizer.hpp"

#include <algorithm>

Tokenizer::Tokenizer(std::vector<Configuration> &&configurations, TokenValue endValue, TokenValue newlineValue)
: mConfigurations(std::move(configurations))
{
    mEndValue = endValue;
    mNewlineValue = newlineValue;
    for(const auto &configuration : mConfigurations) {
        std::vector<std::string> patterns;
        for(const auto &pattern : configuration.patterns) {
            patterns.push_back(pattern.regex);
        }
        mMatchers.push_back(std::make_unique<Regex::Matcher>(std::move(patterns)));
    }
}

unsigned int Tokenizer::patternValue(const std::string &name, unsigned int configuration) const
{
    for(unsigned int i=0; i<mConfigurations[configuration].patterns.size(); i++) {
        if(mConfigurations[configuration].patterns[i].name == name) {
            return mConfigurations[configuration].patterns[i].value;
        }
    }

    return kInvalidTokenValue;
}

Tokenizer::Stream::Stream(const Tokenizer &tokenizer, std::istream &input)
: mTokenizer(tokenizer), mInput(input)
{
    mConsumed = 0;
    mLine = 0;
    mConfiguration = 0;
    mNextToken = {kInvalidTokenValue, 0, 0};
}

void Tokenizer::Stream::setConfiguration(unsigned int configuration)
{
    if(configuration < mTokenizer.mConfigurations.size()) {
        mConfiguration = configuration;
    }
}

unsigned int Tokenizer::Stream::configuration() const
{
    return mConfiguration;
}

const Tokenizer::Token &Tokenizer::Stream::nextToken()
{
    if(mLine == 0) {
        consumeToken();
    }
    return mNextToken;
}

void Tokenizer::Stream::consumeToken()
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
