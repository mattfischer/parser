#include "Tokenizer.hpp"

Tokenizer::Tokenizer(const std::vector<std::string> &patterns, const std::string &input)
: mMatcher(patterns), mInput(input)
{
    mConsumed = 0;
    mEndToken = patterns.size();
    mErrorToken = patterns.size() + 1;

    consumeToken();
}

bool Tokenizer::valid() const
{
    return mMatcher.valid();
}

const Regex::Matcher::ParseError &Tokenizer::regexParseError() const
{
    return mMatcher.parseError();
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