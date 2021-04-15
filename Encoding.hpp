#ifndef ENCODING_HPP
#define ENCODING_HPP

#include <tuple>
#include <vector>

class Encoding
{
public:
    typedef int InputSymbol;
    typedef int CodePoint;

    typedef std::pair<InputSymbol, InputSymbol> InputSymbolRange;

    Encoding(std::vector<InputSymbolRange> &&inputSymbolRanges);

    std::vector<CodePoint> codePoints(InputSymbolRange range) const;
    CodePoint codePoint(InputSymbol symbol) const;

private:
    std::vector<InputSymbolRange> mInputSymbolRanges;
};

#endif