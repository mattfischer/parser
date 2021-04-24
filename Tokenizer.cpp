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
