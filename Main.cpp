#include <iostream>

#include "DefReader.hpp"

int main(int argc, char *argv[])
{
    DefReader reader("grammar.def");
    if(!reader.valid()) {
        return 1;
    }

    if(!reader.matcher().valid()) {
        const Regex::Matcher::ParseError &parseError = reader.matcher().parseError();
        std::cout << "Error, pattern " << parseError.pattern << " character " << parseError.character << ": " << parseError.message << std::endl;
        return 1;
    }

    if(!reader.parser().valid()) {
        std::cout << "Conflict on rule " << reader.parser().conflict().rule << ": " << reader.parser().conflict().rhs1 << " vs " << reader.parser().conflict().rhs2 << std::endl;
        return 1; 
    }

    std::string input = "abc";
    Tokenizer tokenizer(reader.matcher(), input);

    try {
        reader.parser().parse(tokenizer);
    } catch (Parser::ParseException e) {
        std::cout << "Error: Unexpected symbol " << e.symbol << std::endl;
        return 1;
    }

    return 0;
}