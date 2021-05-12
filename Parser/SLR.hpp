#ifndef PARSER_SLR_HPP
#define PARSER_SLR_HPP

#include "LRSingle.hpp"

#include <set>
#include <map>
#include <vector>

namespace Parser {

    class SLR : public LRSingle
    {
    public:
        SLR(const Grammar &grammar);

    private:
    };
}
#endif