#include "Encoding.hpp"

#include <algorithm>
#include <iostream>

namespace Regex {

    std::vector<Encoding::InputSymbolRange> invertRanges(std::vector<Encoding::InputSymbolRange> &ranges)
    {
        std::vector<Encoding::InputSymbolRange> outputRanges;
        std::sort(ranges.begin(), ranges.end(), [](const Encoding::InputSymbolRange &a, const Encoding::InputSymbolRange &b) { return a.first < b.first; });
        Encoding::InputSymbol start = 0;
        for(const auto &range : ranges) {
            if(range.first > start) {
                outputRanges.push_back(Encoding::InputSymbolRange(start, range.first - 1));
            }
            start = range.second + 1;
        }
        outputRanges.push_back(Encoding::InputSymbolRange(start, 127));
        return outputRanges;
    }

    static void visitNode(const Parser::Node &node, std::vector<Encoding::InputSymbolRange> &inputSymbolRanges)
    {
        switch(node.type) {
            case Parser::Node::Type::Symbol:
            {
                const Parser::SymbolNode &symbolNode = static_cast<const Parser::SymbolNode&>(node);
                inputSymbolRanges.push_back(Encoding::InputSymbolRange(symbolNode.symbol, symbolNode.symbol));
                break;
            }

            case Parser::Node::Type::CharacterClass:
            {
                const Parser::CharacterClassNode &characterClassNode = static_cast<const Parser::CharacterClassNode&>(node);
                std::vector<Encoding::InputSymbolRange> ranges;
                for(const auto &range : characterClassNode.ranges) {
                    ranges.push_back(Encoding::InputSymbolRange(range.first, range.second));
                }

                if(characterClassNode.invert) {
                    ranges = invertRanges(ranges);
                }

                inputSymbolRanges.insert(ranges.begin(), ranges.end(), inputSymbolRanges.end());
                break;
            }

            case Parser::Node::Type::OneOf:
            {
                const Parser::OneOfNode &oneOfNode = static_cast<const Parser::OneOfNode&>(node);
                for(const auto &child : oneOfNode.nodes) {
                    visitNode(*child, inputSymbolRanges);
                }
                break;
            }

            case Parser::Node::Type::ZeroOrOne:
            {
                const Parser::ZeroOrOneNode &zeroOrOneNode = static_cast<const Parser::ZeroOrOneNode&>(node);
                visitNode(*zeroOrOneNode.node, inputSymbolRanges);
                break;
            }

            case Parser::Node::Type::ZeroOrMore:
            {
                const Parser::ZeroOrMoreNode &zeroOrMoreNode = static_cast<const Parser::ZeroOrMoreNode&>(node);
                visitNode(*zeroOrMoreNode.node, inputSymbolRanges);
                break;
            }

            case Parser::Node::Type::OneOrMore:
            {
                const Parser::OneOrMoreNode &oneOrMoreNode = static_cast<const Parser::OneOrMoreNode&>(node);
                visitNode(*oneOrMoreNode.node, inputSymbolRanges);
                break;
            }

            case Parser::Node::Type::Sequence:
            {
                const Parser::SequenceNode &sequenceNode = static_cast<const Parser::SequenceNode&>(node);
                for(const auto &child : sequenceNode.nodes) {
                    visitNode(*child, inputSymbolRanges);
                }
                break;
            }
        }
    }

    Encoding::Encoding(const std::vector<std::unique_ptr<Parser::Node>> &nodes)
    {
        std::vector<Encoding::InputSymbolRange> inputSymbolRanges;
        for(const auto &node : nodes) {
            visitNode(*node, inputSymbolRanges);
        }

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
        mInvalidCodePoint = mInputSymbolRanges.size();
        mTotalRange.first = mInputSymbolRanges[0].first;
        mTotalRange.second = mInputSymbolRanges[mInputSymbolRanges.size() - 1].second;

        mSymbolMap.resize(mTotalRange.second - mTotalRange.first + 1, mInvalidCodePoint);
        for(unsigned int i = 0; i<mInputSymbolRanges.size(); i++) {
            const auto &range = mInputSymbolRanges[i];
            for(InputSymbol j = range.first; j <= range.second; j++) {
                mSymbolMap[j - mTotalRange.first] = i;
            }
        }
    }

    std::vector<Encoding::CodePoint> Encoding::codePointRanges(InputSymbolRange inputSymbolRange) const
    {
        std::vector<Encoding::CodePoint> codePoints;
        while(inputSymbolRange.first <= inputSymbolRange.second) {
            auto cmp = [](const InputSymbolRange &a, const InputSymbolRange &b) { return a.first < b.first; };
            auto it = std::lower_bound(mInputSymbolRanges.begin(), mInputSymbolRanges.end(), inputSymbolRange, cmp);
            codePoints.push_back(Encoding::CodePoint(it - mInputSymbolRanges.begin()));
            inputSymbolRange.first = it->second + 1;
        }

        return codePoints;
    }

    Encoding::CodePoint Encoding::codePoint(InputSymbol symbol) const
    {
        if(symbol < mTotalRange.first || symbol > mTotalRange.second) {
            return mInvalidCodePoint;
        }

        return mSymbolMap[symbol - mTotalRange.first];
    }

    unsigned int Encoding::numCodePoints() const
    {
        return (unsigned int)mInputSymbolRanges.size();
    }

    void Encoding::print() const
    {
        for(int i=0; i<mInputSymbolRanges.size(); i++) {
            std::cout << i << ": " << mInputSymbolRanges[i].first;

            if(mInputSymbolRanges[i].second != mInputSymbolRanges[i].first) {
                std::cout << "-" << mInputSymbolRanges[i].second;
            }
            std::cout << std::endl;
        }
    }
}