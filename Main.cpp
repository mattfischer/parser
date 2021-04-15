#include <iostream>
#include <tuple>

#include "Parser.hpp"
#include "NFA.hpp"
#include "DFA.hpp"

int main(int argc, char *argv[])
{
    std::string input = "[a-d]*a";
    std::unique_ptr<Parser::Node> node;
    
    try {
        node = Parser::parse(input);
        std::cout << "**** Parse tree: " << std::endl;
        node->print();
        std::cout << std::endl;
    } catch(Parser::ParseException e) {
        std::cout << "Error, position " << e.pos << ": " << e.message << std::endl;
        return 1;
    }

    NFA nfa(*node);
    std::cout << "**** NFA:" << std::endl;
    nfa.print();
    std::cout << std::endl;

    DFA dfa(nfa);
    std::cout << "**** DFA:" << std::endl;
    dfa.print();
    std::cout << std::endl;

    dfa.minimize();
    std::cout << "**** Minimized DFA:" << std::endl;
    dfa.print();
    std::cout << std::endl;

    return 0;
}