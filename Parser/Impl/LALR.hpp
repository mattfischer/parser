#ifndef PARSER_IMPL_LALR_HPP
#define PARSER_IMPL_LALR_HPP

#include "Parser/Impl/LRSingle.hpp"

namespace Parser
{
    namespace Impl
    {
        class LALR : public LRSingle
        {
        public:
            LALR(const Grammar &grammar);
        };
    }
}
#endif