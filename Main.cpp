#include <iostream>
#include <tuple>

#include "Parser.hpp"
#include "NFA.hpp"
#include "DFA.hpp"
#include "Matcher.hpp"

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

    Encoding encoding(*node);
    std::cout << "**** Encoding:" << std::endl;
    encoding.print();
    std::cout << std::endl;

    NFA nfa(*node, encoding);
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

    Matcher matcher(dfa, encoding);
    unsigned int matched = matcher.match("abcd");
    std::cout << "Matched " << matched << " characters" << std::endl;

    return 0;
}