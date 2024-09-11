#include "MathParser.hpp"

#include "Parser/DefReader.hpp"

#include <iostream>
#include <format>
#include <sstream>

const std::string grammar = R"(
NUMBER: [0-9]+
IGNORE: \s

<root> : <E>
<E>: <T> ( ( '+' | '-' ) <T> )*
<T>: <F> ( ( '*' | '/' ) <F> )*
<F>: NUMBER | '(' <E> ')'
)";

MathParser::MathParser()
{
    std::stringstream grammarStream(grammar);

    mReader = std::make_unique<Parser::DefReader>(grammarStream);
    if(!mReader->valid()) {
        std::cout << std::format("Error in def file, line {}: {}", mReader->parseError().line, mReader->parseError().message) << std::endl;
        return;
    }

    mParser = std::make_unique<Parser::Impl::LL<AstNode>>(mReader->grammar());

    mParser->addTerminalDecorator("NUMBER", [](const Parser::Tokenizer::Token &token) {
        return std::make_unique<AstNodeNumber>(std::atoi(token.text.c_str()));
    });

    mParser->addReducer("root", [](auto rhs) {
        return std::move(rhs.begin()->data);
    });
    unsigned int minus = mReader->grammar().terminalIndex("-");
    mParser->addReducer("E", [=](auto rhs) {
        auto it = rhs.begin();
        std::unique_ptr<AstNode> node = std::move(it->data);
        ++it;
        while(it != rhs.end()) {
            AstNode::Type type = AstNode::Type::Add;
            if(it->index == minus) {
                type = AstNode::Type::Subtract;
            }
            ++it;
            node = std::make_unique<AstNode>(type, std::move(node), std::move(it->data));
            ++it;
        }
        return node;
    });
    unsigned int divide = mReader->grammar().terminalIndex("/");
    mParser->addReducer("T", [=](auto rhs) {
        auto it = rhs.begin();
        std::unique_ptr<AstNode> node = std::move(it->data);
        ++it;
        while(it != rhs.end()) {
            AstNode::Type type = AstNode::Type::Multiply;
            if(it->index == divide) {
                type = AstNode::Type::Divide;
            }
            ++it;
            node = std::make_unique<AstNode>(type, std::move(node), std::move(it->data));
            ++it;
        }
        return node;
    });
    unsigned int lparen = mReader->grammar().terminalIndex("(");
    mParser->addReducer("F", [=](auto rhs) {
        auto it = rhs.begin();
        if(it->index == lparen) {
            ++it;
        }
        return std::move(it->data);
    });
}

std::unique_ptr<MathParser::AstNode> MathParser::parse(const std::string &input) const
{
    std::stringstream ss(input);
    Parser::Tokenizer::Stream stream(mReader->tokenizer(), ss);

    std::unique_ptr<AstNode> ast = mParser->parse(stream);
    if(!ast) {
        std::cout << std::format("Error: Unexpected symbol {}", stream.nextToken().text) << std::endl;
    }
    return ast;
}

int MathParser::evaluate(const AstNode &node) const
{
    switch(node.type) {
        case AstNode::Type::Number:
            return static_cast<const AstNodeNumber&>(node).number;
        case AstNode::Type::Add:
            return evaluate(*node.children[0]) + evaluate(*node.children[1]);
        case AstNode::Type::Subtract:
            return evaluate(*node.children[0]) - evaluate(*node.children[1]);
        case AstNode::Type::Multiply:
            return evaluate(*node.children[0]) * evaluate(*node.children[1]);
        case AstNode::Type::Divide:
            return evaluate(*node.children[0]) / evaluate(*node.children[1]);
    }
    return 0;
}
