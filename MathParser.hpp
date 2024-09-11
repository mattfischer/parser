#ifndef MATH_PARSER_HPP
#define MATH_PARSER_HPP

#include "Parser/DefReader.hpp"
#include "Parser/Impl/LL.hpp"

#include <memory>
#include <vector>

class MathParser {
public:
    MathParser();

    struct AstNode
    {
        enum class Type {
            Number,
            Add,
            Subtract,
            Multiply,
            Divide
        };

        template<typename ...Children> AstNode(Type t, Children... c) : type(t)
        {
            children.reserve(sizeof...(c));
            (children.push_back(std::move(c)), ...);
        }

        Type type;
        std::vector<std::shared_ptr<AstNode>> children;
    };

    struct AstNodeNumber : public AstNode
    {
        AstNodeNumber(int n) : AstNode(Type::Number), number(n) {}

        int number;
    };

    std::unique_ptr<AstNode> parse(const std::string &input) const;

    int evaluate(const AstNode &node) const;

private:
    std::unique_ptr<Parser::DefReader> mReader;
    std::unique_ptr<Parser::Impl::LL<AstNode>> mParser;
};

#endif