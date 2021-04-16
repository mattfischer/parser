#include <iostream>

#include "Regex/Matcher.hpp"

int main(int argc, char *argv[])
{
    std::string pattern = "a?b";
    Regex::Matcher matcher(pattern);
    if(!matcher.valid()) {
        std::cout << "Error: " << matcher.parseErrorMessage() << std::endl;
        return 1;
    }
 
    unsigned int matched = matcher.match("bcda");
    std::cout << "Matched " << matched << " characters" << std::endl;

    return 0;
}