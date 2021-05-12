#ifndef PARSER_TOMITA_HPP
#define PARSER_TOMITA_HPP

#include "Parser/LRMulti.hpp"

namespace Parser
{
    class Tomita : public LRMulti
    {
    public:
        Tomita(const Grammar &grammar);
    };
}
#endif