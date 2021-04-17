#ifndef REGEX_MATCHER_HPP
#define REGEX_MATCHER_HPP

#include "DFA.hpp"
#include "Encoding.hpp"

#include <memory>
#include <string>

namespace Regex {

    class Matcher {
    public:
        Matcher(const std::vector<std::string> &patterns);

        bool valid() const;

        struct ParseError {
            unsigned int pattern;
            unsigned int character;
            std::string message;
        };
        const ParseError &parseError() const;

        unsigned int match(const std::string &string, unsigned int start, unsigned int &pattern) const;

    private:
        std::unique_ptr<DFA> mDFA;
        std::unique_ptr<Encoding> mEncoding;
        ParseError mParseError;
    };
}

#endif