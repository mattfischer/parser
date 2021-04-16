#ifndef REGEX_MATCHER_HPP
#define REGEX_MATCHER_HPP

#include "DFA.hpp"
#include "Encoding.hpp"

#include <memory>
#include <string>

namespace Regex {

    class Matcher {
    public:
        Matcher(const std::string &pattern);

        bool valid() const;
        const std::string &parseErrorMessage() const;

        unsigned int match(const std::string &string) const;

    private:
        std::unique_ptr<DFA> mDFA;
        std::unique_ptr<Encoding> mEncoding;
        std::string mParseErrorMessage;
    };
}

#endif