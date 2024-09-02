#include <iostream>
#include <sstream>

#include "Parser/DefReader.hpp"
#include "Parser/Impl/LALR.hpp"

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

int evaluate(const AstNode &node)
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

int main(int argc, char *argv[])
{
    Parser::DefReader reader("grammar.def");
    if(!reader.valid()) {
        std::cout << "Error in def file, line " << reader.parseError().line << ": " << reader.parseError().message << std::endl;
        return 1;
    }

    Parser::Impl::LALR<AstNode> parser(reader.grammar());

    parser.addTerminalDecorator("NUMBER", [](const Parser::Tokenizer::Token &token) {
        return std::make_unique<AstNodeNumber>(std::atoi(token.text.c_str()));
    });

    parser.addReducer("root", [](auto rhs) {
        return std::move(rhs.begin()->data);
    });
    unsigned int minus = reader.grammar().terminalIndex("-");
    parser.addReducer("E", [&](auto rhs) {
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
    unsigned int divide = reader.grammar().terminalIndex("/");
    parser.addReducer("T", [&](auto rhs) {
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
    unsigned int lparen = reader.grammar().terminalIndex("(");
    parser.addReducer("F", [&](auto rhs) {
        auto it = rhs.begin();
        if(it->index == lparen) {
            ++it;
        }
        return std::move(it->data);
    });

    while(true) {
        std::string input;
        std::cout << ": ";
        std::getline(std::cin, input);
        if(input.size() == 0) {
            break;
        }

        std::stringstream ss(input);
        Parser::Tokenizer::Stream stream(reader.tokenizer(), ss);

        std::unique_ptr<AstNode> ast = parser.parse(stream);
        if(ast) {
            int result = evaluate(*ast);
            std::cout << result << std::endl;
        } else {
            std::cout << "Error: Unexpected symbol " << stream.nextToken().text << std::endl;
        }
    }

    return 0;
}