#include "Tokenizer.hpp"

Tokenizer::Tokenizer(const Regex::Matcher &matcher, unsigned int ignorePattern, const std::string &input)
: mMatcher(matcher), mInput(input)
{
    mConsumed = 0;
    mEndToken = mMatcher.numPatterns();
    mErrorToken = mMatcher.numPatterns() + 1;
    mIgnorePattern = ignorePattern;

    consumeToken();
}

const Tokenizer::Token &Tokenizer::nextToken() const
{
    return mNextToken;
}

void Tokenizer::consumeToken()
{
    if(mNextToken.index == mErrorToken) {
        return;
    }

    if(mConsumed == mInput.size()) {
        mNextToken.index = mEndToken;
        mNextToken.start = mConsumed;
        mNextToken.length = 0;
        return;
    }

    bool repeat = true;
    while(repeat) {
        unsigned int pattern;
        unsigned int matched = mMatcher.match(mInput, mConsumed, pattern);
        if(matched == 0) {
            mNextToken.index = mErrorToken;
            mNextToken.start = mConsumed;
            mNextToken.length = 0;

            repeat = false;
        } else {
            mConsumed += matched;

            if(pattern != mIgnorePattern) {
                mNextToken.index = pattern;
                mNextToken.start = mConsumed;
                mNextToken.length = matched;

                repeat = false;
            }
        }
    }
}

unsigned int Tokenizer::endToken() const
{
    return mEndToken;
}


unsigned int Tokenizer::errorToken() const
{
    return mErrorToken;
}