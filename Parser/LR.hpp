#ifndef PARSER_LR_HPP
#define PARSER_LR_HPP

#include "Parser/Grammar.hpp"
#include "Util/Table.hpp"

#include "Tokenizer.hpp"

namespace Parser
{
    class LR
    {
    public:
        LR(const Grammar &grammar);

        bool valid() const;
    
        const Grammar &grammar() const;

        struct Conflict {
            enum class Type {
                ShiftReduce,
                ReduceReduce
            };
            Type type;
            unsigned int symbol;
            unsigned int item1;
            unsigned int item2;
        };

        const Conflict &conflict() const;

        template<typename Data> struct ParseItem {
            enum class Type {
                Terminal,
                Nonterminal
            };
            Type type;
            unsigned int index;
            std::unique_ptr<Data> data;
        };

        template<typename ParseData> class ParseSession
        {
        public:
            typedef std::function<std::unique_ptr<ParseData>(const Tokenizer::Token&)> TerminalDecorator;
            typedef std::function<std::unique_ptr<ParseData>(ParseItem<ParseData>*, unsigned int)> Reducer;
            
            ParseSession(const LR &parser)
            : mParser(parser)
            {
            }
        
            void addTerminalDecorator(const std::string &terminal, TerminalDecorator terminalDecorator)
            {
                unsigned int terminalIndex = mParser.grammar().terminalIndex(terminal);
                if(terminalIndex != UINT_MAX) {
                    mTerminalDecorators[terminalIndex] = terminalDecorator;
                }
            }

            void addReducer(const std::string &rule, Reducer reducer)
            {
                unsigned int ruleIndex = mParser.grammar().ruleIndex(rule);
                if(ruleIndex != UINT_MAX) {
                    mReducers[ruleIndex] = reducer;
                }
            }

            std::unique_ptr<ParseData> parse(Tokenizer::Stream &stream)
            {
                struct StateItem {
                    unsigned int state;
                    unsigned int parseStackStart;
                };

                std::vector<StateItem> stateStack;
                std::vector<ParseItem<ParseData>> parseStack;
                unsigned int state = 0;

                while(mParser.mAcceptStates.count(state) == 0) {
                    stateStack.push_back(StateItem{state, (unsigned int)parseStack.size()});
                    const ParseTableEntry &entry = mParser.mParseTable.at(state, mParser.terminalIndex(stream.nextToken().value));
                    switch(entry.type) {
                        case ParseTableEntry::Type::Shift:
                        {
                            ParseItem<ParseData> parseItem;
                            parseItem.type = ParseItem<ParseData>::Type::Terminal;
                            parseItem.index = stream.nextToken().value;
                            auto it = mTerminalDecorators.find(stream.nextToken().value);
                            if(it != mTerminalDecorators.end()) {
                                parseItem.data = it->second(stream.nextToken());
                            }
                            parseStack.push_back(std::move(parseItem));

                            stream.consumeToken();
                            state = entry.index;
                            break;
                        }

                        case ParseTableEntry::Type::Reduce:
                        {
                            const Reduction &reduction = mParser.mReductions[entry.index];
                            const Grammar::RHS &rhs = mParser.mGrammar.rules()[reduction.rule].rhs[reduction.rhs];
                            for(unsigned int i=0; i<rhs.size(); i++) {
                                if(rhs[i].type != Grammar::Symbol::Type::Epsilon) {
                                    stateStack.pop_back();
                                }
                            }

                            state = stateStack.back().state;
                            unsigned int parseStackStart = stateStack.back().parseStackStart;
        
                            auto it = mReducers.find(reduction.rule);
                            if(it != mReducers.end()) {
                                std::unique_ptr<ParseData> data = it->second(&parseStack[parseStackStart], (unsigned int)(parseStack.size() - parseStackStart));
                                parseStack.erase(parseStack.begin() + parseStackStart, parseStack.end());

                                ParseItem<ParseData> parseItem;
                                parseItem.type = ParseItem<ParseData>::Type::Nonterminal;
                                parseItem.index = reduction.rule;
                                parseItem.data = std::move(data);
                                parseStack.push_back(std::move(parseItem));
                            }
                            
                            const ParseTableEntry &newEntry = mParser.mParseTable.at(state, mParser.ruleIndex(reduction.rule));
                            state = newEntry.index;
                            break;
                        }

                        case ParseTableEntry::Type::Error:
                            return std::unique_ptr<ParseData>();
                    }
                }

                std::unique_ptr<ParseData> result;
                auto it = mReducers.find(mParser.mGrammar.startRule());
                if(it != mReducers.end()) {
                    result = it->second(&parseStack[0], (unsigned int)parseStack.size());
                }

                return std::move(result);
            }

        private:
            const LR &mParser;
            std::map<unsigned int, TerminalDecorator> mTerminalDecorators;
            std::map<unsigned int, Reducer> mReducers;
        };

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
        bool computeParseTable(const std::vector<State> &states, GetReduceLookahead getReduceLookahead);

        unsigned int symbolIndex(const Grammar::Symbol &symbol) const;
        unsigned int terminalIndex(unsigned int terminal) const;
        unsigned int ruleIndex(unsigned int rule) const;

        struct ParseTableEntry {
            enum class Type {
                Shift,
                Reduce,
                Error
            };
            Type type;
            unsigned int index;
        };

        struct Reduction {
            bool operator==(const Reduction &other) {
                return rule == other.rule && rhs == other.rhs;
            }

            unsigned int rule;
            unsigned int rhs;
        };

        const Grammar &mGrammar;
        Util::Table<ParseTableEntry> mParseTable;
        std::vector<Reduction> mReductions;
        std::set<unsigned int> mAcceptStates;
        bool mValid;
        Conflict mConflict;
    };
}
#endif