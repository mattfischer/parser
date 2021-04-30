#include <iostream>
#include <sstream>

#include "DefReader.hpp"
#include "LLParser.hpp"

struct NumberData
{
    NumberData(int n) : number(n) {}

    int number;
};

struct AstNode
{
    enum class Type {
        Number,
        Add,
        Subtract,
        Multiply,
        Divide
    };

    template<typename ...Children> AstNode(Type t, Children&&... c) : type(t)
    {
        children.reserve(sizeof...(c));
        (children.push_back(std::move(c)), ...);
    }

    Type type;
    std::vector<std::unique_ptr<AstNode>> children;
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
}

int main(int argc, char *argv[])
{
    DefReader reader("grammar.def");
    if(!reader.valid()) {
        std::cout << "Error in def file, line " << reader.parseError().line << ": " << reader.parseError().message << std::endl;
        return 1;
    }

    LLParser parser(reader.grammar());
    if(!parser.valid()) {
        std::cout << "Conflict on rule " << parser.conflict().rule << ": " << parser.conflict().rhs1 << " vs " << parser.conflict().rhs2 << std::endl;
        return 1; 
    }
    
    while(true) {
        std::string input;
        std::cout << ": ";
        std::getline(std::cin, input);
        if(input.size() == 0) {
            break;
        }

        std::stringstream ss(input);
        Tokenizer::Stream<NumberData> stream(reader.tokenizer(), ss);
        stream.addDecorator("NUMBER", 0, [](const std::string &text) {
            return std::make_unique<NumberData>(std::atoi(text.c_str()));
        });

        LLParser::ParseSession<AstNode, NumberData> session(parser);
        session.addTerminalDecorator("NUMBER", [](const NumberData &numberData) {
            return std::make_unique<AstNodeNumber>(numberData.number);
        });

        session.addReducer("root", [](LLParser::ParseItem<AstNode> *items, unsigned int numItems) {
            return std::move(items[0].data);
        });
        unsigned int minus = reader.grammar().terminalIndex("-");
        session.addReducer("E", [&](LLParser::ParseItem<AstNode> *items, unsigned int numItems) {
            std::unique_ptr<AstNode> node = std::move(items[0].data);
            for(unsigned int i=1; i<numItems; i+=2) {
                AstNode::Type type = AstNode::Type::Add;
                if(items[i].index == minus) {
                    type = AstNode::Type::Subtract;
                }
                node = std::make_unique<AstNode>(type, std::move(node), std::move(items[i+1].data));
            }
            return node;
        });
        unsigned int divide = reader.grammar().terminalIndex("/");
        session.addReducer("T", [&](LLParser::ParseItem<AstNode> *items, unsigned int numItems) {
            std::unique_ptr<AstNode> node = std::move(items[0].data);
            for(unsigned int i=1; i<numItems; i+=2) {
                AstNode::Type type = AstNode::Type::Multiply;
                if(items[i].index == divide) {
                    type = AstNode::Type::Divide;
                }
                node = std::make_unique<AstNode>(type, std::move(node), std::move(items[i+1].data));
            }
            return node;
        });
        session.addReducer("F", [](LLParser::ParseItem<AstNode> *items, unsigned int numItems) {
            if(numItems == 1) {
                return std::move(items[0].data);
            } else {
                return std::move(items[1].data);
            }
        });

        try {
            std::unique_ptr<AstNode> ast = session.parse(stream);
            int result = evaluate(*ast);
            std::cout << result << std::endl;
        } catch (LLParser::ParseException e) {
            std::cout << "Error: Unexpected symbol " << e.symbol << std::endl;
            return 1;
        }
    }

    return 0;
}