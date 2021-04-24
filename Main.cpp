#include <iostream>
#include <sstream>

#include "DefReader.hpp"
#include "LLParser.hpp"

struct NumberData
{
    NumberData(int n) : number(n) {}

    int number;
};

unsigned int numberToken;
std::unique_ptr<NumberData> decorateToken(unsigned int index, const std::string &text)
{
    if(index == numberToken) {
        return std::make_unique<NumberData>(std::atoi(text.c_str()));
    }

    return nullptr;
}

struct AstNode
{
    enum class Type {
        Number,
        Add
    };
    AstNode(Type t) : type(t) {}

    Type type;
    std::vector<std::unique_ptr<AstNode>> children;
};

struct AstNodeNumber : public AstNode
{
    AstNodeNumber(int n) : AstNode(Type::Number), number(n) {}

    int number;
};

std::unique_ptr<AstNode> decorateTerminal(const Tokenizer::Token<NumberData> &token)
{
    if(token.index == 1) {
        return std::make_unique<AstNodeNumber>(token.data->number);
    }

    return nullptr;
}

unsigned int ERule;
unsigned int rootRule;

std::unique_ptr<AstNode> reduce(std::vector<LLParser::ParseItem<AstNode>> &parseStack, unsigned int parseStart, unsigned int rule, unsigned int rhs)
{
    if(rule == ERule) {
        std::unique_ptr<AstNode> node = std::move(parseStack[parseStart].data);
        for(unsigned int i=parseStart + 2; i<parseStack.size(); i+=2) {
            std::unique_ptr<AstNode> b = std::move(parseStack[i].data);
            std::unique_ptr<AstNode> result = std::make_unique<AstNode>(AstNode::Type::Add);
            result->children.push_back(std::move(node));
            result->children.push_back(std::move(b));
            node = std::move(result);
        }
        return node;
    } else if(rule == rootRule) {
        return std::move(parseStack[parseStart].data);
    }

    return nullptr;
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
    
    numberToken = reader.tokenizer().patternIndex("NUMBER", 0);
    ERule = parser.grammar().ruleIndex("<E>");
    rootRule = parser.grammar().ruleIndex("<root>");

    std::stringstream ss("2345 + 2 + 5");
    Tokenizer::Stream<NumberData> stream(reader.tokenizer(), ss, decorateToken);

    try {
        std::unique_ptr<AstNode> ast = parser.parse<AstNode, NumberData>(stream, decorateTerminal, reduce);
    } catch (LLParser::ParseException e) {
        std::cout << "Error: Unexpected symbol " << e.symbol << std::endl;
        return 1;
    }

    return 0;
}