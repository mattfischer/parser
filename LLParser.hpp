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
    
        void addTerminalDecorator(unsigned int index, TerminalDecorator terminalDecorator)
        {
            mTerminalDecorators[index] = terminalDecorator;
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
            std::vector<ParseItem<ParseData>> parseStack;
            parseRule(mParser.mGrammar.startRule(), stream, parseStack);
        
            return std::move(parseStack[0].data);
        }

    private:
        void parseRule(unsigned int rule, Tokenizer::Stream<TokenData> &stream, std::vector<ParseItem<ParseData>> &parseStack) const
        {
            unsigned int rhs = mParser.rhs(rule, stream.nextToken().index);

            if(rhs == UINT_MAX) {
                throw ParseException(stream.nextToken().index);
            }   

            unsigned int parseStackStart = (unsigned int)parseStack.size();
            const std::vector<Grammar::Symbol> &symbols = mParser.grammar().rules()[rule].rhs[rhs];
            for(unsigned int i=0; i<symbols.size(); i++) {
                const Grammar::Symbol &symbol = symbols[i];
                switch(symbol.type) {
                    case Grammar::Symbol::Type::Terminal:
                        if(stream.nextToken().index == symbol.index) {
                            ParseItem<ParseData> parseItem;
                            parseItem.type = ParseItem<ParseData>::Type::Terminal;
                            parseItem.index = symbol.index;
                            auto it = mTerminalDecorators.find(symbol.index);
                            if(it != mTerminalDecorators.end()) {
                                parseItem.data = it->second(*stream.nextToken().data);
                            }
                            parseStack.push_back(std::move(parseItem));
                            auto it2 = mMatchListeners.find(std::pair<unsigned int, unsigned int>(rule, rhs));
                            if(it2 != mMatchListeners.end()) {
                                it2->second(i);
                            }
                            stream.consumeToken();
                        } else {
                            throw ParseException(stream.nextToken().index);
                        }
                        break;

                    case Grammar::Symbol::Type::Nonterminal:
                        parseRule(symbol.index, stream, parseStack);
                        break;
                }
            }

            if(parseStackStart < parseStack.size()) {
                auto it = mReducers.find(std::pair<unsigned int, unsigned int>(rule, rhs));
                if(it != mReducers.end()) {
                    std::unique_ptr<ParseData> data = it->second(&parseStack[parseStackStart], (unsigned int)(parseStack.size() - parseStackStart));
                    parseStack.erase(parseStack.begin() + parseStackStart, parseStack.end());

                    ParseItem<ParseData> parseItem;
                    parseItem.type = ParseItem<ParseData>::Type::Nonterminal;
                    parseItem.index = rule;
                    parseItem.data = std::move(data);
                    parseStack.push_back(std::move(parseItem));
                }
            }
        }

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