#include <iostream>

#include "Regex/Matcher.hpp"

int main(int argc, char *argv[])
{
    std::vector<std::string> patterns;
    patterns.push_back("a?be");
    patterns.push_back("adc");
    Regex::Matcher matcher(patterns);
    if(!matcher.valid()) {
        std::cout << "Error: " << matcher.parseErrorMessage() << std::endl;
        return 1;
    }
 
    unsigned int pattern;
    unsigned int matched = matcher.match("be", pattern);
    std::cout << "Matched " << matched << " characters (pattern " << pattern << ")" << std::endl;

    return 0;
}