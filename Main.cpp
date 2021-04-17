#include <iostream>

#include "Tokenizer.hpp"

int main(int argc, char *argv[])
{
    std::vector<std::string> patterns{"[0-9]+", "[a-zA-Z][a-zA-Z0-9]*", " "};
    std::string input = "12345 abc123 abc";
    Tokenizer tokenizer(patterns, input);
    if(!tokenizer.valid()) {
        const Regex::Matcher::ParseError &parseError = tokenizer.regexParseError();
        std::cout << "Error, pattern " << parseError.pattern << " character " << parseError.character << ": " << parseError.message << std::endl;
        return 1;
    }

    while(true) {
        std::string substr = input.substr(tokenizer.nextToken().start, tokenizer.nextToken().length);
        std::cout << "Token " << tokenizer.nextToken().index << ": " << substr << std::endl;
        
        if(tokenizer.nextToken().index == tokenizer.endToken() || tokenizer.nextToken().index == tokenizer.errorToken()) {
            break;
        }

        tokenizer.consumeToken();
    }

    return 0;
}