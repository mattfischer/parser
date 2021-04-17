#include <iostream>

#include "Tokenizer.hpp"
#include "Parser.hpp"

int main(int argc, char *argv[])
{
    std::vector<std::string> patterns{"a", "b"};
    Regex::Matcher matcher(patterns);

    if(!matcher.valid()) {
        const Regex::Matcher::ParseError &parseError = matcher.parseError();
        std::cout << "Error, pattern " << parseError.pattern << " character " << parseError.character << ": " << parseError.message << std::endl;
        return 1;
    }

    std::vector<Parser::Rule> rules {
        Parser::Rule{ std::vector<Parser::RHS>{ 
            Parser::RHS{ std::vector<Parser::Symbol>{ 
                Parser::Symbol{ Parser::Symbol::Type::Terminal, 0 }, 
                Parser::Symbol{ Parser::Symbol::Type::Terminal, 1 }
            } } 
        } }
    };

    Parser parser(rules);

    std::string input = "ab";
    Tokenizer tokenizer(matcher, input);

    parser.parse(tokenizer);

    return 0;
}