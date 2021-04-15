#include "Encoding.hpp"

#include <algorithm>

Encoding::Encoding(std::vector<InputSymbolRange> &&inputSymbolRanges)
{
    auto cmp = [](const InputSymbolRange &a, const InputSymbolRange &b) { return a.first < b.first; };
    std::sort(inputSymbolRanges.begin(), inputSymbolRanges.end(), cmp);
    InputSymbolRange current = inputSymbolRanges.front();
    inputSymbolRanges.erase(inputSymbolRanges.begin());
    while(inputSymbolRanges.size() > 0) {
        InputSymbolRange next = inputSymbolRanges.front();
        inputSymbolRanges.erase(inputSymbolRanges.begin());

        if(current.second < next.first) {
            mInputSymbolRanges.push_back(current);
            current = next;
            continue;
        }
 
        if(next.first > current.first) {
            InputSymbolRange head(current.first, next.first - 1);
            mInputSymbolRanges.push_back(head);
            current.first = head.first;
        }

        if(current.second != next.second) {
            InputSymbolRange rest(std::min(current.second, next.second) + 1, std::max(current.second, next.second));
            auto it = std::upper_bound(inputSymbolRanges.begin(), inputSymbolRanges.end(), rest, cmp);
            inputSymbolRanges.insert(it, rest);
        }

        current.second = std::min(current.second, next.second);
    }

    mInputSymbolRanges.push_back(current);
}

std::vector<Encoding::CodePoint> Encoding::codePoints(InputSymbolRange inputSymbolRange) const
{
    std::vector<Encoding::CodePoint> codePoints;
    while(inputSymbolRange.first <= inputSymbolRange.second) {
        auto cmp = [](const InputSymbolRange &a, const InputSymbolRange &b) { return a.first < b.first; };
        auto it = std::lower_bound(mInputSymbolRanges.begin(), mInputSymbolRanges.end(), inputSymbolRange, cmp);
        codePoints.push_back(it - mInputSymbolRanges.begin());
        inputSymbolRange.first = it->second + 1;
    }

    return codePoints;
}

Encoding::CodePoint Encoding::codePoint(InputSymbol symbol) const
{
    return codePoints(InputSymbolRange(symbol, symbol))[0];
}