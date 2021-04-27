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
        Add
    };
    AstNode(Type t) : type(t) {}
    template<typename ...Children> AstNode(Type t, Children&&... c)
    {
        type = t;
        addChildren(std::forward<Children&&>(c)...);
    }

    void addChildren() {}
    template<typename ...Children> void addChildren(std::unique_ptr<AstNode> &&firstChild, Children&&... otherChildren)
    {
        children.push_back(std::move(firstChild));
        addChildren(std::forward<Children&&>(otherChildren)...);
    }

    Type type;
    std::vector<std::unique_ptr<AstNode>> children;
};

struct AstNodeNumber : public AstNode
{
    AstNodeNumber(int n) : AstNode(Type::Number), number(n) {}

    int number;
};

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
    
    std::stringstream ss("2345 + 2 + 5");
    Tokenizer::Stream<NumberData> stream(reader.tokenizer(), ss);
    stream.addDecorator("NUMBER", 0, [](const std::string &text) -> std::unique_ptr<NumberData> {
        return std::make_unique<NumberData>(std::atoi(text.c_str()));
    });

    LLParser::ParseSession<AstNode, NumberData> session(parser);
    session.addTerminalDecorator("NUMBER", [](const NumberData &numberData) -> std::unique_ptr<AstNode> {
        return std::make_unique<AstNodeNumber>(numberData.number);
    });

    session.addReducer("<root>", 0, [](LLParser::ParseItem<AstNode> *items, unsigned int numItems) -> std::unique_ptr<AstNode> {
        return std::move(items[0].data);
    });
    session.addReducer("<E>", 0, [](LLParser::ParseItem<AstNode> *items, unsigned int numItems) -> std::unique_ptr<AstNode> {
        std::unique_ptr<AstNode> node = std::move(items[0].data);
        for(unsigned int i=2; i<numItems; i+=2) {
            node = std::make_unique<AstNode>(AstNode::Type::Add, std::move(node), std::move(items[i].data));
        }
        return node;
    });

    try {
        std::unique_ptr<AstNode> ast = session.parse(stream);
    } catch (LLParser::ParseException e) {
        std::cout << "Error: Unexpected symbol " << e.symbol << std::endl;
        return 1;
    }

    return 0;
}