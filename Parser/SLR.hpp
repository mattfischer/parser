#ifndef PARSER_SLR_HPP
#define PARSER_SLR_HPP

#include "LR.hpp"

#include <set>
#include <map>
#include <vector>

namespace Parser {

    class SLR : public LR
    {
    public:
        SLR(const Grammar &grammar);

    private:
    };
}
#endif