#ifndef PARSER_IMPL_LL_HPP
#define PARSER_IMPL_LL_HPP

#include "Parser/Base.hpp"
#include "Parser/Tokenizer.hpp"

#include "Util/Table.hpp"

#include <vector>
#include <set>

namespace Parser
{
    namespace Impl
    {
        class LL : public Base {
        public:
            LL(const Grammar &grammar);

            bool valid() const;

            struct Conflict {
                unsigned int rule;
                unsigned int symbol;
                unsigned int rhs1;
                unsigned int rhs2;
            };
            const Conflict &conflict() const;

            unsigned int rhs(unsigned int rule, unsigned int symbol) const;

            template<typename ParseData> class ParseSession
            {
            public:
                struct ParseItem {
                    enum class Type {
                        Terminal,
                        Nonterminal
                    };
                    Type type;
                    unsigned int index;
                    std::unique_ptr<ParseData> data;

                    typedef ParseItem* iterator;
                };

                typedef std::function<std::unique_ptr<ParseData>(const Tokenizer::Token&)> TerminalDecorator;
                typedef std::function<std::unique_ptr<ParseData>(typename ParseItem::iterator, typename ParseItem::iterator)> Reducer;
                typedef std::function<void(unsigned int)> MatchListener;

                ParseSession(const LL &parser);
            
                void addMatchListener(const std::string &rule, MatchListener matchListener);        
                void addTerminalDecorator(const std::string &terminal, TerminalDecorator terminalDecorator);
                void addReducer(const std::string &rule, Reducer reducer);

                std::unique_ptr<ParseData> parse(Tokenizer::Stream &stream) const;

            private:
                const LL &mParser;
                std::map<unsigned int, MatchListener> mMatchListeners;
                std::map<unsigned int, TerminalDecorator> mTerminalDecorators;
                std::map<unsigned int, Reducer> mReducers;
            };

        private:
            bool addParseTableEntry(unsigned int rule, unsigned int symbol, unsigned int rhs);
            bool addParseTableEntries(unsigned int rule, const std::set<unsigned int> &symbols, unsigned int rhs);
            bool computeParseTable(const std::vector<std::set<unsigned int>> &firstSets, std::vector<std::set<unsigned int>> &followSets, std::set<unsigned int> &nullableNonterminals);
        
            Util::Table<unsigned int> mParseTable;  
            bool mValid;
            Conflict mConflict;
        };

        template<typename ParseData> LL::ParseSession<ParseData>::ParseSession(const LL &parser)
        : mParser(parser)
        {
        }

        template<typename ParseData> void LL::ParseSession<ParseData>::addMatchListener(const std::string &rule, MatchListener matchListener)
        {
            unsigned int ruleIndex = mParser.grammar().ruleIndex(rule);
            if(ruleIndex != UINT_MAX) {
                mMatchListeners[ruleIndex] = matchListener;
            }
        }

        template<typename ParseData> void LL::ParseSession<ParseData>::addTerminalDecorator(const std::string &terminal, TerminalDecorator terminalDecorator)
        {
            unsigned int terminalIndex = mParser.grammar().terminalIndex(terminal);
            if(terminalIndex != UINT_MAX) {
                mTerminalDecorators[terminalIndex] = terminalDecorator;
            }
        }

        template<typename ParseData> void LL::ParseSession<ParseData>::addReducer(const std::string &rule, Reducer reducer)
        {
            unsigned int ruleIndex = mParser.grammar().ruleIndex(rule);
            if(ruleIndex != UINT_MAX) {
                mReducers[ruleIndex] = reducer;
            }
        }

        template<typename ParseData> std::unique_ptr<ParseData> LL::ParseSession<ParseData>::parse(Tokenizer::Stream &stream) const
        {
            struct PredictItem {
                enum class Type {
                    Terminal,
                    Nonterminal,
                    Reduce
                };
                Type type;
                union {
                    struct {
                        unsigned int index;
                        unsigned int rule;
                        unsigned int pos;
                    } symbol;
                    struct {
                        unsigned int rule;
                        unsigned int parseStackStart;
                    } reduce;
                };
            };

            std::vector<PredictItem> predictStack;
            std::vector<ParseItem> parseStack;

            predictStack.push_back(PredictItem{PredictItem::Type::Nonterminal, mParser.mGrammar.startRule()});

            while(predictStack.size() > 0) {
                PredictItem predictItem = predictStack.back();
                predictStack.pop_back();

                switch(predictItem.type) {
                    case PredictItem::Type::Terminal:
                    {
                        if(stream.nextToken().value == predictItem.symbol.index) {
                            ParseItem parseItem;
                            parseItem.type = ParseItem::Type::Terminal;
                            parseItem.index = predictItem.symbol.index;
                            auto it = mTerminalDecorators.find(predictItem.symbol.index);
                            if(it != mTerminalDecorators.end()) {
                                parseItem.data = it->second(stream.nextToken());
                            }
                            parseStack.push_back(std::move(parseItem));
                            auto it2 = mMatchListeners.find(predictItem.symbol.rule);
                            if(it2 != mMatchListeners.end()) {
                                it2->second(predictItem.symbol.pos);
                            }
                            stream.consumeToken();
                        } else {
                            return std::unique_ptr<ParseData>();
                        }
                        break;
                    }

                    case PredictItem::Type::Nonterminal:
                    {
                        unsigned int nextRule = predictItem.symbol.index;
                        unsigned int nextRhs = mParser.rhs(nextRule, stream.nextToken().value);

                        if(nextRhs == UINT_MAX) {
                            return std::unique_ptr<ParseData>();
                        }   

                        if(mReducers.find(nextRule) != mReducers.end()) {
                            predictStack.push_back(PredictItem{PredictItem::Type::Reduce, nextRule, (unsigned int)parseStack.size()});
                        }

                        const std::vector<Grammar::Symbol> &symbols = mParser.grammar().rules()[nextRule].rhs[nextRhs];
                        for(unsigned int i=0; i<symbols.size(); i++) {
                            unsigned int ri = (unsigned int)symbols.size() - i - 1;
                            const Grammar::Symbol &s = symbols[ri];
                            switch(s.type) {
                                case Grammar::Symbol::Type::Terminal:
                                    predictStack.push_back(PredictItem{PredictItem::Type::Terminal, s.index, nextRule, ri});
                                    break;
                                
                                case Grammar::Symbol::Type::Nonterminal:
                                    predictStack.push_back(PredictItem{PredictItem::Type::Nonterminal, s.index, nextRule, ri});
                                    break;
                                
                                case Grammar::Symbol::Type::Epsilon:
                                    break;
                            }
                        }
                        break;
                    }

                    case PredictItem::Type::Reduce:
                    {
                        unsigned int currentRule = predictItem.reduce.rule;
                        unsigned int parseStackStart = predictItem.reduce.parseStackStart;

                        auto it = mReducers.find(currentRule);
                        if(it != mReducers.end()) {
                            std::unique_ptr<ParseData> data = it->second(&parseStack[parseStackStart], &parseStack[0] + parseStack.size());
                            parseStack.erase(parseStack.begin() + parseStackStart, parseStack.end());

                            ParseItem parseItem;
                            parseItem.type = ParseItem::Type::Nonterminal;
                            parseItem.index = currentRule;
                            parseItem.data = std::move(data);
                            parseStack.push_back(std::move(parseItem));
                        }
                        break;
                    }
                }
            }
        
            return std::move(parseStack[0].data);
        }
    }
}

#endif