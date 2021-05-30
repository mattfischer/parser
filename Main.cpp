#include <iostream>
#include <sstream>

#include "DefReader.hpp"
#include "Parser/Earley.hpp"

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
    DefReader reader("grammar.def");
    if(!reader.valid()) {
        std::cout << "Error in def file, line " << reader.parseError().line << ": " << reader.parseError().message << std::endl;
        return 1;
    }

    Parser::Earley parser(reader.grammar());
    
    Parser::Earley::ParseSession<AstNode> session(parser);

    session.addTerminalDecorator("NUMBER", [](const Tokenizer::Token &token) {
        return std::make_unique<AstNodeNumber>(std::atoi(token.text.c_str()));
    });

    session.addReducer("root", [](auto begin, auto end) {
        return std::move(begin->data);
    });
    unsigned int minus = reader.grammar().terminalIndex("-");
    session.addReducer("E", [&](auto begin, auto end) {
        auto it = begin;
        std::shared_ptr<AstNode> node = it->data;
        ++it;
        while(it != end) {
            AstNode::Type type = AstNode::Type::Add;
            if(it->index == minus) {
                type = AstNode::Type::Subtract;
            }
            ++it;
            node = std::make_unique<AstNode>(type, node, it->data);
            ++it;
        }
        return node;
    });
    unsigned int divide = reader.grammar().terminalIndex("/");
    session.addReducer("T", [&](auto begin, auto end) {
        auto it = begin;
        std::shared_ptr<AstNode> node = it->data;
        ++it;
        while(it != end) {
            AstNode::Type type = AstNode::Type::Multiply;
            if(it->index == divide) {
                type = AstNode::Type::Divide;
            }
            ++it;
            node = std::make_unique<AstNode>(type, node, it->data);
            ++it;
        }
        return node;
    });
    unsigned int lparen = reader.grammar().terminalIndex("(");
    session.addReducer("F", [&](auto begin, auto end) {
        auto it = begin;
        if(it->index == lparen) {
            ++it;
        }
        return it->data;
    });

    while(true) {
        std::string input;
        std::cout << ": ";
        std::getline(std::cin, input);
        if(input.size() == 0) {
            break;
        }

        std::stringstream ss(input);
        Tokenizer::Stream stream(reader.tokenizer(), ss);

        std::vector<std::shared_ptr<AstNode>> ast = session.parse(stream);
        if(ast.size() > 0) {
            for(const auto &tree : ast) {
                int result = evaluate(*tree);
                std::cout << result << std::endl;
            }
        } else {
            std::cout << "Error: Unexpected symbol " << stream.nextToken().text << std::endl;
        }
    }

    return 0;
}