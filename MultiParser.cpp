#include "MultiParser.hpp"

#include "Parser/DefReader.hpp"

#include <iostream>
#include <format>
#include <sstream>

const std::string grammar = R"(
NUMBER: [0-9]+
IGNORE: \s

<root> : <E>
<E>: NUMBER | <E> '+' <E>
)";

MultiParser::MultiParser()
{
    std::stringstream grammarStream(grammar);

    mReader = std::make_unique<Parser::DefReader>(grammarStream);
    if(!mReader->valid()) {
        std::cout << std::format("Error in def file, line {}: {}", mReader->parseError().line, mReader->parseError().message) << std::endl;
        return;
    }

    mParser = std::make_unique<Parser::Impl::GLR<AstNode>>(mReader->grammar());

    mParser->addTerminalDecorator("NUMBER", [](const Parser::Tokenizer::Token &token) {
        return std::make_shared<AstNodeNumber>(std::atoi(token.text.c_str()));
    });

    mParser->addReducer("root", [](auto rhs) {
        return rhs.begin()->data;
    });
    mParser->addReducer("E", [=](auto rhs) {
        auto it = rhs.begin();
        std::shared_ptr<AstNode> node = it->data;
        ++it;
        while(it != rhs.end()) {
            AstNode::Type type = AstNode::Type::Add;
            ++it;
            node = std::make_shared<AstNode>(type, node, it->data);
            ++it;
        }
        return node;
    });
}

std::vector<std::shared_ptr<MultiParser::AstNode>> MultiParser::parse(const std::string &input) const
{
    std::stringstream ss(input);
    Parser::Tokenizer::Stream stream(mReader->tokenizer(), ss);

    std::vector<std::shared_ptr<AstNode>> trees = mParser->parse(stream);

    return trees;
}

int MultiParser::evaluate(const AstNode &node) const
{
    switch(node.type) {
        case AstNode::Type::Number:
            return static_cast<const AstNodeNumber&>(node).number;
        case AstNode::Type::Add:
            return evaluate(*node.children[0]) + evaluate(*node.children[1]);
    }
    return 0;
}
