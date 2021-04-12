#include <iostream>
#include <tuple>

#include "Parser.hpp"
#include "NFA.hpp"

int main(int argc, char *argv[])
{
    std::string input = "abcd";
    std::unique_ptr<Parser::Node> node;
    
    try {
        node = Parser::parse(input);
        Parser::printNode(*node);
    } catch(Parser::ParseException e) {
        std::cout << "Error, position " << e.pos << ": " << e.message << std::endl;
        return 1;
    }

    std::unique_ptr<NFA> nfa = std::make_unique<NFA>(*node);

    return 0;
}