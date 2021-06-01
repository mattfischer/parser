#ifndef PARSER_IMPL_SLR_HPP
#define PARSER_IMPL_SLR_HPP

#include "Parser/Impl/LRSingle.hpp"

#include <set>
#include <map>
#include <vector>

namespace Parser
{
    namespace Impl
    {
        class SLR : public LRSingle
        {
        public:
            SLR(const Grammar &grammar);

        private:
        };
    }
}
#endif