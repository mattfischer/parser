#ifndef MATCHER_HPP
#define MATCHER_HPP

#include "DFA.hpp"
#include "Encoding.hpp"

class Matcher {
public:
    Matcher(const DFA &dfa, const Encoding &encoding);

    unsigned int match(const std::string &string) const;

private:
    const DFA &mDFA;
    const Encoding &mEncoding;
};

#endif