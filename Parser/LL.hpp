#ifndef PARSER_LL_HPP
#define PARSER_LL_HPP

#include "Parser/Grammar.hpp"
#include "Util/Table.hpp"

#include "Tokenizer.hpp"

#include <vector>
#include <set>

namespace Parser {

    class LL {
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

        const Grammar &grammar() const;
        unsigned int rhs(unsigned int rule, unsigned int symbol) const;

        template<typename Data> struct ParseItem {
            enum class Type {
                Terminal,
                Nonterminal
            };
            Type type;
            unsigned int index;
            std::unique_ptr<Data> data;

            typedef ParseItem* iterator;
        };

        template<typename ParseData> class ParseSession
        {
        public:
            typedef std::function<std::unique_ptr<ParseData>(const Tokenizer::Token&)> TerminalDecorator;
            typedef std::function<std::unique_ptr<ParseData>(typename ParseItem<ParseData>::iterator, typename ParseItem<ParseData>::iterator)> Reducer;
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
    
        const Grammar &mGrammar;
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
        struct SymbolItem {
            enum class Type {
                Terminal,
                Nonterminal,
                Reduce
            };
            Type type;
            unsigned int index;
        };

        struct RuleItem {
            unsigned int rule;
            unsigned int parseStackStart;
        };

        std::vector<ParseItem<ParseData>> parseStack;
        std::vector<SymbolItem> symbolStack;
        std::vector<RuleItem> ruleStack;

        symbolStack.push_back(SymbolItem{SymbolItem::Type::Nonterminal, mParser.mGrammar.startRule()});

        while(symbolStack.size() > 0) {
            SymbolItem symbolItem = symbolStack.back();
            symbolStack.pop_back();

            switch(symbolItem.type) {
                case SymbolItem::Type::Terminal:
                {
                    unsigned int currentRule = ruleStack.back().rule;
                    unsigned int parseStackStart = ruleStack.back().parseStackStart;
                    unsigned int currentSymbol = (unsigned int)(parseStack.size() - parseStackStart);

                    if(stream.nextToken().value == symbolItem.index) {
                        ParseItem<ParseData> parseItem;
                        parseItem.type = ParseItem<ParseData>::Type::Terminal;
                        parseItem.index = symbolItem.index;
                        auto it = mTerminalDecorators.find(symbolItem.index);
                        if(it != mTerminalDecorators.end()) {
                            parseItem.data = it->second(stream.nextToken());
                        }
                        parseStack.push_back(std::move(parseItem));
                        auto it2 = mMatchListeners.find(currentRule);
                        if(it2 != mMatchListeners.end()) {
                            it2->second(currentSymbol);
                        }
                        stream.consumeToken();
                    } else {
                        return std::unique_ptr<ParseData>();
                    }
                    break;
                }

                case SymbolItem::Type::Nonterminal:
                {
                    unsigned int nextRule = symbolItem.index;
                    unsigned int nextRhs = mParser.rhs(nextRule, stream.nextToken().value);

                    if(nextRhs == UINT_MAX) {
                        return std::unique_ptr<ParseData>();
                    }   

                    if(mReducers.find(nextRule) != mReducers.end()) {
                        ruleStack.push_back(RuleItem{nextRule, (unsigned int)parseStack.size()});
                        symbolStack.push_back(SymbolItem{SymbolItem::Type::Reduce, nextRule});
                    }

                    const std::vector<Grammar::Symbol> &symbols = mParser.grammar().rules()[nextRule].rhs[nextRhs];
                    for(auto it = symbols.rbegin(); it != symbols.rend(); it++) {
                        const Grammar::Symbol &s = *it;
                        switch(s.type) {
                            case Grammar::Symbol::Type::Terminal:
                                symbolStack.push_back(SymbolItem{SymbolItem::Type::Terminal, s.index});
                                break;
                            
                            case Grammar::Symbol::Type::Nonterminal:
                                symbolStack.push_back(SymbolItem{SymbolItem::Type::Nonterminal, s.index});
                                break;
                            
                            case Grammar::Symbol::Type::Epsilon:
                                break;
                        }
                    }
                    break;
                }

                case SymbolItem::Type::Reduce:
                {
                    unsigned int currentRule = ruleStack.back().rule;
                    unsigned int parseStackStart = ruleStack.back().parseStackStart;

                    auto it = mReducers.find(currentRule);
                    if(it != mReducers.end()) {
                        std::unique_ptr<ParseData> data = it->second(&parseStack[parseStackStart], &parseStack[0] + parseStack.size());
                        parseStack.erase(parseStack.begin() + parseStackStart, parseStack.end());

                        ParseItem<ParseData> parseItem;
                        parseItem.type = ParseItem<ParseData>::Type::Nonterminal;
                        parseItem.index = currentRule;
                        parseItem.data = std::move(data);
                        parseStack.push_back(std::move(parseItem));

                        ruleStack.pop_back();
                    }
                    break;
                }
            }
        }
    
        return std::move(parseStack[0].data);
    }
}

#endif