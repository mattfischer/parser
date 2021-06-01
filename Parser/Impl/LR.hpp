#ifndef PARSER_IMPL_LR_HPP
#define PARSER_IMPL_LR_HPP

#include "Parser/Base.hpp"
#include "Parser/Tokenizer.hpp"

#include "Util/Table.hpp"

namespace Parser
{
    namespace Impl
    {
        class LR : public Base
        {
        public:
            LR(const Grammar &grammar);
        
        protected:
            struct Item {
                bool operator<(const Item &other) const;
                bool operator==(const Item &other) const;

                unsigned int rule;
                unsigned int rhs;
                unsigned int pos;
            };

            struct State {
                std::set<Item> items;
                std::map<unsigned int, unsigned int> transitions;
            };

            void computeClosure(std::set<Item> &items) const;
            std::vector<State> computeStates() const;

            typedef std::function<std::set<unsigned int>(unsigned int, unsigned int)> GetReduceLookahead;

            void printStates(const std::vector<State> &states, GetReduceLookahead getReduceLookahead) const;

            unsigned int symbolIndex(const Grammar::Symbol &symbol) const;
            unsigned int terminalIndex(unsigned int terminal) const;
            unsigned int ruleIndex(unsigned int rule) const;
        };
    }
}
#endif