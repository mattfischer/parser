#include "Tokenizer.hpp"

#include <algorithm>

Tokenizer::Tokenizer(std::vector<Configuration> &&configurations)
: mConfigurations(std::move(configurations))
{
    initialize();
}

Tokenizer::Tokenizer(Regex::Matcher &&matcher, unsigned int ignorePattern, std::vector<std::string> &&patternNames)
{
    mConfigurations.push_back(Configuration{std::move(matcher), ignorePattern, std::move(patternNames)});
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

unsigned int Tokenizer::patternIndex(const std::string &name, unsigned int configuration) const
{
    for(unsigned int i=0; i<mConfigurations[configuration].patternNames.size(); i++) {
        if(mConfigurations[configuration].patternNames[i] == name) {
            return i;
        }
    }

    return UINT_MAX;
}