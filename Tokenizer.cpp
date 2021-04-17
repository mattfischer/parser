#include "Tokenizer.hpp"

Tokenizer::Tokenizer(const Regex::Matcher &matcher, const std::string &input)
: mMatcher(matcher), mInput(input)
{
    mConsumed = 0;
    mEndToken = mMatcher.numPatterns();
    mErrorToken = mMatcher.numPatterns() + 1;

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

    unsigned int pattern;
    unsigned int matched = mMatcher.match(mInput, mConsumed, pattern);
    if(matched == 0) {
        mNextToken.index = mErrorToken;
        mNextToken.start = mConsumed;
        mNextToken.length = 0;
    } else {
        mNextToken.index = pattern;
        mNextToken.start = mConsumed;
        mNextToken.length = matched;

        mConsumed += matched;
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