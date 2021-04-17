#ifndef REGEX_ENCODING_HPP
#define REGEX_ENCODING_HPP

#include "Parser.hpp"

#include <tuple>
#include <vector>
#include <map>

namespace Regex {

    class Encoding
    {
    public:
        typedef char InputSymbol;
        typedef int CodePoint;

        typedef std::pair<InputSymbol, InputSymbol> InputSymbolRange;

        Encoding(const std::vector<std::unique_ptr<Parser::Node>> &nodes);

        std::vector<CodePoint> codePointRanges(InputSymbolRange range) const;
        CodePoint codePoint(InputSymbol symbol) const;
        unsigned int numCodePoints() const;

        void print() const;

    private:
        std::vector<InputSymbolRange> mInputSymbolRanges;
        InputSymbolRange mTotalRange;
        CodePoint mInvalidCodePoint;
        std::vector<CodePoint> mSymbolMap;
    };
}

#endif