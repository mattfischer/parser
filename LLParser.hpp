#ifndef LLPARSER_HPP
#define LLPARSER_HPP

#include "Grammar.hpp"
#include "Tokenizer.hpp"

#include <vector>
#include <set>

class LLParser {
public:
    class ParseException : public std::exception {
    public:
        ParseException(unsigned int s) { symbol = s; }

        unsigned int symbol;
    };

    LLParser(const Grammar &grammar);

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
    };

    template<typename ParseData, typename TokenData> class ParseSession
    {
    public:
        typedef std::function<std::unique_ptr<ParseData>(const TokenData&)> TerminalDecorator;
        typedef std::function<std::unique_ptr<ParseData>(ParseItem<ParseData>*, unsigned int)> Reducer;
        typedef std::function<void(unsigned int)> MatchListener;

        ParseSession(const LLParser &parser)
        : mParser(parser)
        {
        }
    
        void addMatchListener(const std::string &rule, unsigned int rhs, MatchListener matchListener)
        {
            unsigned int ruleIndex = mParser.grammar().ruleIndex(rule);
            if(ruleIndex != UINT_MAX) {
                mMatchListeners[std::pair<unsigned int, unsigned int>(ruleIndex, rhs)] = matchListener;
            }
        }
    
        void addTerminalDecorator(const std::string &terminal, TerminalDecorator terminalDecorator)
        {
            unsigned int terminalIndex = mParser.grammar().terminalIndex(terminal);
            if(terminalIndex != UINT_MAX) {
                mTerminalDecorators[terminalIndex] = terminalDecorator;
            }
        }

        void addReducer(const std::string &rule, unsigned int rhs, Reducer reducer)
        {
            unsigned int ruleIndex = mParser.grammar().ruleIndex(rule);
            if(ruleIndex != UINT_MAX) {
                mReducers[std::pair<unsigned int, unsigned int>(ruleIndex, rhs)] = reducer;
            }
        }

        std::unique_ptr<ParseData> parse(Tokenizer::Stream<TokenData> &stream) const
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
                unsigned int rhs;
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
                        unsigned int currentRhs = ruleStack.back().rhs;
                        unsigned int parseStackStart = ruleStack.back().parseStackStart;
                        unsigned int currentSymbol = (unsigned int)(parseStack.size() - parseStackStart);

                        if(stream.nextToken().value == symbolItem.index) {
                            ParseItem<ParseData> parseItem;
                            parseItem.type = ParseItem<ParseData>::Type::Terminal;
                            parseItem.index = symbolItem.index;
                            auto it = mTerminalDecorators.find(symbolItem.index);
                            if(it != mTerminalDecorators.end()) {
                                parseItem.data = it->second(*stream.nextToken().data);
                            }
                            parseStack.push_back(std::move(parseItem));
                            auto it2 = mMatchListeners.find(std::pair<unsigned int, unsigned int>(currentRule, currentRhs));
                            if(it2 != mMatchListeners.end()) {
                                it2->second(currentSymbol);
                            }
                            stream.consumeToken();
                        } else {
                            throw ParseException(stream.nextToken().value);
                        }
                        break;
                    }

                    case SymbolItem::Type::Nonterminal:
                    {
                        unsigned int nextRule = symbolItem.index;
                        unsigned int nextRhs = mParser.rhs(nextRule, stream.nextToken().value);

                        if(nextRhs == UINT_MAX) {
                            throw ParseException(stream.nextToken().value);
                        }   

                        if(mReducers.find(std::pair<unsigned int, unsigned int>(nextRule, nextRhs)) != mReducers.end()) {
                            ruleStack.push_back(RuleItem{nextRule, nextRhs, (unsigned int)parseStack.size()});
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
                        unsigned int currentRhs = ruleStack.back().rhs;
                        unsigned int parseStackStart = ruleStack.back().parseStackStart;

                        auto it = mReducers.find(std::pair<unsigned int, unsigned int>(currentRule, currentRhs));
                        if(it != mReducers.end()) {
                            std::unique_ptr<ParseData> data = it->second(&parseStack[parseStackStart], (unsigned int)(parseStack.size() - parseStackStart));
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

    private:
        const LLParser &mParser;
        std::map<std::pair<unsigned int, unsigned int>, MatchListener> mMatchListeners;
        std::map<unsigned int, TerminalDecorator> mTerminalDecorators;
        std::map<std::pair<unsigned int, unsigned int>, Reducer> mReducers;
    };

private:
    bool addParseTableEntry(unsigned int rule, unsigned int symbol, unsigned int rhs);
    bool addParseTableEntries(unsigned int rule, const std::set<unsigned int> &symbols, unsigned int rhs);
    bool computeParseTable(const std::vector<std::set<unsigned int>> &firstSets, std::vector<std::set<unsigned int>> &followSets, std::set<unsigned int> &nullableNonterminals);
   
    const Grammar &mGrammar;
    std::vector<unsigned int> mParseTable;  
    unsigned int mNumSymbols;
    bool mValid;
    Conflict mConflict;
};

#endif