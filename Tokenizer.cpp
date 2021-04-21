#include "Tokenizer.hpp"

Tokenizer::Tokenizer(const Regex::Matcher &matcher, unsigned int ignorePattern)
: mMatcher(matcher)
{
    mEndToken = mMatcher.numPatterns();
    mErrorToken = mMatcher.numPatterns() + 1;
    mIgnorePattern = ignorePattern;
}

unsigned int Tokenizer::endToken() const
{
    return mEndToken;
}

unsigned int Tokenizer::errorToken() const
{
    return mErrorToken;
}

Tokenizer::Stream::Stream(const Tokenizer &tokenizer, const std::string &input)
: mTokenizer(tokenizer), mInput(input)
{
    mConsumed = 0;
    
    consumeToken();
}

const Tokenizer::Token &Tokenizer::Stream::nextToken() const
{
    return mNextToken;
}

void Tokenizer::Stream::consumeToken()
{
    if(mNextToken.index == mTokenizer.mErrorToken) {
        return;
    }

    if(mConsumed == mInput.size()) {
        mNextToken.index = mTokenizer.mEndToken;
        mNextToken.start = mConsumed;
        mNextToken.length = 0;
        return;
    }

    bool repeat = true;
    while(repeat) {
        unsigned int pattern;
        unsigned int matched = mTokenizer.mMatcher.match(mInput, mConsumed, pattern);
        if(matched == 0) {
            mNextToken.index = mTokenizer.mErrorToken;
            mNextToken.start = mConsumed;
            mNextToken.length = 0;

            repeat = false;
        } else {
            mConsumed += matched;

            if(pattern != mTokenizer.mIgnorePattern) {
                mNextToken.index = pattern;
                mNextToken.start = mConsumed;
                mNextToken.length = matched;

                repeat = false;
            }
        }
    }
}
