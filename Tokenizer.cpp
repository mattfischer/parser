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