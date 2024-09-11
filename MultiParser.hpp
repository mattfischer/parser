#ifndef MULTI_PARSER_HPP
#define MULTI_PARSER_HPP

#include "Parser/DefReader.hpp"
#include "Parser/Impl/GLR.hpp"

#include <memory>
#include <vector>

class MultiParser {
public:
    MultiParser();

    struct AstNode
    {
        enum class Type {
            Number,
            Add
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

    std::vector<std::shared_ptr<AstNode>> parse(const std::string &input) const;

    int evaluate(const AstNode &node) const;

private:
    std::unique_ptr<Parser::DefReader> mReader;
    std::unique_ptr<Parser::Impl::GLR<AstNode>> mParser;
};

#endif