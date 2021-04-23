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

struct AstNode : public LLParser::ParseItem::Data
{
    enum class Type {
        Number
    };
    Type type;
};

struct AstNodeNumber : public AstNode
{
    AstNodeNumber(int n) : number(n) {}

    int number;
};

void decorateTerminal(LLParser::ParseItem &parseItem, const Tokenizer::Token &token)
{
    if(parseItem.index == 1) {
        parseItem.data = std::make_unique<AstNodeNumber>(static_cast<const NumberData&>(*token.data).number);
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

    std::stringstream ss("2345 + 2 + 5");
    Tokenizer::Stream stream(reader.tokenizer(), ss, decorateToken);

    try {
        parser.parse(stream, decorateTerminal);
    } catch (LLParser::ParseException e) {
        std::cout << "Error: Unexpected symbol " << e.symbol << std::endl;
        return 1;
    }

    return 0;
}