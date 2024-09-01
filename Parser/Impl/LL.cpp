#include "Parser/Impl/LL.hpp"

#include <algorithm>

namespace Parser::Impl
{
    LLBase::LLBase(const Grammar &grammar)
    : Base(grammar)
    {
        std::vector<std::set<unsigned int>> firstSets;
        std::vector<std::set<unsigned int>> followSets;
        std::set<unsigned int> nullableNonterminals;
        mGrammar.computeSets(firstSets, followSets, nullableNonterminals);

        mValid = computeParseTable(firstSets, followSets, nullableNonterminals);
    }

    bool LLBase::addParseTableEntry(unsigned int rule, unsigned int symbol, unsigned int rhs)
    {
        if(mParseTable.at(rule, symbol) == UINT_MAX) {
            mParseTable.at(rule, symbol) = rhs;
            return true;
        } else {
            mConflict.rule = rule;
            mConflict.symbol = symbol;
            mConflict.rhs1 = mParseTable.at(rule, symbol);
            mConflict.rhs2 = rhs;
            return false;
        }
    }

    bool LLBase::addParseTableEntries(unsigned int rule, const std::set<unsigned int> &symbols, unsigned int rhs)
    {
        for(unsigned int s : symbols) {
            if(!addParseTableEntry(rule, s, rhs)) {
                return false;
            }
        }

        return true;
    }

    bool LLBase::computeParseTable(const std::vector<std::set<unsigned int>> &firstSets, std::vector<std::set<unsigned int>> &followSets, std::set<unsigned int> &nullableNonterminals)
    {
        mParseTable.resize(mGrammar.rules().size(), mGrammar.terminals().size(), UINT_MAX);

        for(unsigned int i=0; i<mGrammar.rules().size(); i++) {
            const Grammar::Rule &rule = mGrammar.rules()[i];

            for(unsigned int j=0; j<rule.rhs.size(); j++) {
                const Grammar::Symbol &symbol = rule.rhs[j][0];
                switch(symbol.type) {
                    case Grammar::Symbol::Type::Terminal:
                        if(!addParseTableEntry(i, symbol.index, j)) {
                            return false;
                        }
                        break;
                    
                    case Grammar::Symbol::Type::Nonterminal:
                        if(!addParseTableEntries(i, firstSets[symbol.index], j)) {
                            return false;
                        }

                        if(nullableNonterminals.contains(symbol.index)) {
                            if(!addParseTableEntries(i, followSets[symbol.index], j)) {
                                return false;
                            }
                        }
                        break;
                    
                    case Grammar::Symbol::Type::Epsilon:
                        if(!addParseTableEntries(i, followSets[i], j)) {
                            return false;
                        }
                        break;
                }
            }
        }

        return true;
    }

    bool LLBase::valid() const
    {
        return mValid;
    }

    const LLBase::Conflict &LLBase::conflict() const
    {
        return mConflict;
    }

    unsigned int LLBase::rhs(unsigned int rule, unsigned int symbol) const
    {
        if(symbol == Tokenizer::kErrorTokenValue) {
            return UINT_MAX;
        } else {
            return mParseTable.at(rule, symbol);
        }
    }

    bool LLBase::runParse(Tokenizer::Stream &stream, ParseStackBase &parseStack) const
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

        predictStack.push_back(PredictItem{PredictItem::Type::Nonterminal, grammar().startRule()});

        while(predictStack.size() > 0) {
            PredictItem predictItem = predictStack.back();
            predictStack.pop_back();

            switch(predictItem.type) {
                case PredictItem::Type::Terminal:
                {
                    if(stream.nextToken().value == predictItem.symbol.index) {
                        shift(stream.nextToken(), parseStack);
                        
                        auto it2 = mMatchListeners.find(predictItem.symbol.rule);
                        if(it2 != mMatchListeners.end()) {
                            it2->second(predictItem.symbol.pos);
                        }
                        stream.consumeToken();
                    } else {
                        return false;
                    }
                    break;
                }

                case PredictItem::Type::Nonterminal:
                {
                    unsigned int nextRule = predictItem.symbol.index;
                    unsigned int nextRhs = rhs(nextRule, stream.nextToken().value);

                    if(nextRhs == UINT_MAX) {
                        return false;
                    }   

                    if(canReduce(nextRule)) {
                        predictStack.push_back(PredictItem{PredictItem::Type::Reduce, nextRule, (unsigned int)parseStack.size()});
                    }

                    const std::vector<Grammar::Symbol> &symbols = grammar().rules()[nextRule].rhs[nextRhs];
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

                    reduce(currentRule, parseStackStart, parseStack);
                    break;
                }
            }
        }
    
        return true;
    }
}