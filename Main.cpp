#include <iostream>
#include <format>

#include "MathParser.hpp"
#include "MultiParser.hpp"

void parseMath(const MathParser &parser, const std::string &input)
{
    std::unique_ptr<MathParser::AstNode> ast = parser.parse(input);

    if(ast) {
        int result = parser.evaluate(*ast);
        std::cout << result << std::endl;
    }
}

void parseMulti(const MultiParser &parser, const std::string &input)
{
    std::vector<std::shared_ptr<MultiParser::AstNode>> trees = parser.parse(input);

    for(const auto &ast : trees) {
        int result = parser.evaluate(*ast);
        std::cout << result << std::endl;
    }
}

int main(int argc, char *argv[])
{
    MathParser mathParser;
    MultiParser multiParser;

    while(true) {
        std::string input;
        std::cout << ": ";
        std::getline(std::cin, input);
        if(input.size() == 0) {
            break;
        }

        parseMath(mathParser, input);
        //parseMulti(multiParser, input);
    }

    return 0;
}