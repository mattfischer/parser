#include <iostream>
#include <sstream>

#include "DefReader.hpp"
#include "LLParser.hpp"

int main(int argc, char *argv[])
{
    DefReader reader("grammar.def");
    if(!reader.valid()) {
        std::cout << "Error in def file, line " << reader.parseError().line << ": " << reader.parseError().message << std::endl;
        return 1;
    }

    if(!reader.matcher().valid()) {
        const Regex::Matcher::ParseError &parseError = reader.matcher().parseError();
        std::cout << "Error, pattern " << parseError.pattern << " character " << parseError.character << ": " << parseError.message << std::endl;
        return 1;
    }

    LLParser parser(reader.grammar());
    if(!parser.valid()) {
        std::cout << "Conflict on rule " << parser.conflict().rule << ": " << parser.conflict().rhs1 << " vs " << parser.conflict().rhs2 << std::endl;
        return 1; 
    }

    std::stringstream ss("2345 + 2 + 5");
    Tokenizer::Stream stream(reader.tokenizer(), ss);

    try {
        parser.parse(stream);
    } catch (LLParser::ParseException e) {
        std::cout << "Error: Unexpected symbol " << e.symbol << std::endl;
        return 1;
    }

    return 0;
}