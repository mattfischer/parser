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

Tokenizer::Stream::Stream(const Tokenizer &tokenizer, std::istream &input)
: mTokenizer(tokenizer), mInput(input)
{
    mConsumed = 0;
    mLine = 0;

    consumeToken();
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
                mNextToken.text = "#";
                return;
            }

            std::getline(mInput, mCurrentLine);
            mConsumed = 0;
            mLine++;
        }

        unsigned int pattern;
        unsigned int matched = mTokenizer.mMatcher.match(mCurrentLine, mConsumed, pattern);
        if(matched == 0) {
            mNextToken.index = mTokenizer.mErrorToken;
            mNextToken.start = mConsumed;
            mNextToken.line = mLine;

            repeat = false;
        } else {
            if(pattern != mTokenizer.mIgnorePattern) {
                mNextToken.index = pattern;
                mNextToken.start = mConsumed;
                mNextToken.line = mLine;
                mNextToken.text = mCurrentLine.substr(mConsumed, matched);

                repeat = false;
            }

            mConsumed += matched;
        }
    }
}
