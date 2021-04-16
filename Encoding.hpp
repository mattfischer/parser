#ifndef ENCODING_HPP
#define ENCODING_HPP

#include "Parser.hpp"

#include <tuple>
#include <vector>
#include <map>

class Encoding
{
public:
    typedef char InputSymbol;
    typedef int CodePoint;

    typedef std::pair<InputSymbol, InputSymbol> InputSymbolRange;

    Encoding(const Parser::Node &node);

    std::vector<CodePoint> codePointRanges(InputSymbolRange range) const;
    CodePoint codePoint(InputSymbol symbol) const;
    unsigned int numCodePoints() const;

    void print() const;

private:
    std::vector<InputSymbolRange> mInputSymbolRanges;
    std::map<InputSymbol, CodePoint> mSymbolMap;
};

#endif