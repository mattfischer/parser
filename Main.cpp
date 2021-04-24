#include <iostream>
#include <sstream>

#include "DefReader.hpp"
#include "LLParser.hpp"

struct NumberData : public Tokenizer::Token::Data
{
    NumberData(int n) : number(n) {}

    int number;
};

void decorateToken(Tokenizer::Token &token, const std::string &text)
{
    if(token.index == 1) {
        token.data = std::make_unique<NumberData>(std::atoi(text.c_str()));
    }
}

struct AstNode
{
    enum class Type {
        Number,
        Add
    };
    Type type;
};

struct AstNodeNumber : public AstNode
{
    AstNodeNumber(int n) : number(n) { type = Type::Number; }

    int number;
};

struct AstNodeAdd : public AstNode
{
    AstNodeAdd(std::unique_ptr<AstNode> _a, std::unique_ptr<AstNode> _b) : a(std::move(_a)), b(std::move(_b)) { type = Type::Add; }

    std::unique_ptr<AstNode> a;
    std::unique_ptr<AstNode> b;
};

std::unique_ptr<AstNode> decorateTerminal(const Tokenizer::Token &token)
{
    if(token.index == 1) {
        return std::make_unique<AstNodeNumber>(static_cast<const NumberData&>(*token.data).number);
    }

    return nullptr;
}

std::unique_ptr<AstNode> reduce(std::vector<LLParser::ParseItem<AstNode>> &parseStack, unsigned int parseStart, unsigned int rule, unsigned int rhs)
{
    if(rule == 0) {
        std::unique_ptr<AstNode> node = std::move(parseStack[parseStart].data);
        for(unsigned int i=parseStart + 2; i<parseStack.size(); i+=2) {
            std::unique_ptr<AstNode> b = std::move(parseStack[i].data);
            node = std::make_unique<AstNodeAdd>(std::move(node), std::move(b));
        }
        return node;
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

    std::stringstream ss("2345 + 2 + 5");
    Tokenizer::Stream stream(reader.tokenizer(), ss, decorateToken);

    try {
        parser.parse<AstNode>(stream, decorateTerminal, reduce);
    } catch (LLParser::ParseException e) {
        std::cout << "Error: Unexpected symbol " << e.symbol << std::endl;
        return 1;
    }

    return 0;
}