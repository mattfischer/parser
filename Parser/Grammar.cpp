#include "Grammar.hpp"

#include <iostream>

namespace Parser {

    Grammar::Grammar(std::vector<std::string> terminals, std::vector<Rule> rules, unsigned int startRule)
    : mTerminals(std::move(terminals)), mRules(std::move(rules)), mStartRule(startRule)
    {
    }

    const std::vector<Grammar::Rule> &Grammar::rules() const
    {
        return mRules;
    }

    unsigned int Grammar::startRule() const
    {
        return mStartRule;
    }

    const std::vector<std::string> &Grammar::terminals() const
    {
        return mTerminals;
    }

    unsigned int Grammar::terminalIndex(const std::string &name) const
    {
        for(unsigned int i=0; i<mTerminals.size(); i++) {
            if(mTerminals[i] == name) {
                return i;
            }
        }

        return UINT_MAX;
    }

    unsigned int Grammar::ruleIndex(const std::string &name) const
    {
        for(unsigned int i=0; i<mRules.size(); i++) {
            if(mRules[i].lhs == name) {
                return i;
            }
        }

        return UINT_MAX;
    }

    bool isNullable(const Grammar::Symbol &symbol, const std::set<unsigned int> &nullableNonterminals) {
        switch(symbol.type) {
            case Grammar::Symbol::Type::Terminal:
                return false;
            case Grammar::Symbol::Type::Epsilon:
                return true;
            case Grammar::Symbol::Type::Nonterminal:
                return nullableNonterminals.count(symbol.index) > 0;
        }
        return false;
    }

    void addSymbol(std::set<unsigned int> &set, unsigned int symbol, bool &changed)
    {
        if(set.count(symbol) == 0) {
            set.insert(symbol);
            changed = true;
        }
    }

    void addSet(std::set<unsigned int> &set, const std::set<unsigned int> &source, bool &changed)
    {
        for(unsigned int s : source) {
            addSymbol(set, s, changed);
        }
    }

    void addFirstSet(std::set<unsigned int> &set, const Grammar::Symbol &symbol, const std::vector<std::set<unsigned int>> &firstSets, bool &changed)
    {
        switch(symbol.type) {
            case Grammar::Symbol::Type::Terminal:
                addSymbol(set, symbol.index, changed);
                break;
            
            case Grammar::Symbol::Type::Nonterminal:
                addSet(set, firstSets[symbol.index], changed);
                break;
        }
    }

    void addFirstSet(std::set<unsigned int> &set, 
                    std::vector<Grammar::Symbol>::const_iterator begin,
                    std::vector<Grammar::Symbol>::const_iterator end,
                    const std::vector<std::set<unsigned int>> &firstSets,
                    const std::set<unsigned int> &nullableNonterminals,
                    bool &nullable,
                    bool &changed)
    {
        nullable = true;
        for(auto it = begin; it != end; it++) {
            const Grammar::Symbol &symbol = *it;
            addFirstSet(set, symbol, firstSets, changed);
            if(!isNullable(symbol, nullableNonterminals)) {
                nullable = false;
                break;
            }
        }
    }

    void Grammar::computeSets(std::vector<std::set<unsigned int>> &firstSets, std::vector<std::set<unsigned int>> &followSets, std::set<unsigned int> &nullableNonterminals) const
    {
        firstSets.resize(mRules.size());
        followSets.resize(mRules.size());

        bool changed = true;
        while(changed) {
            changed = false;
            for(unsigned int i=0; i<mRules.size(); i++) {
                for(const RHS &rhs : mRules[i].rhs) {
                    bool nullable;
                    addFirstSet(firstSets[i], rhs.cbegin(), rhs.cend(), firstSets, nullableNonterminals, nullable, changed);
                    if(nullable) {
                        addSymbol(nullableNonterminals, i, changed);
                    }

                    for(unsigned int j=0; j<rhs.size(); j++) {
                        const Symbol &symbol = rhs[j];
                        if(symbol.type == Symbol::Type::Nonterminal) {
                            bool nullable;
                            addFirstSet(followSets[symbol.index], rhs.cbegin() + j + 1, rhs.cend(), firstSets, nullableNonterminals, nullable, changed);
                            if(nullable) {
                                addSet(followSets[symbol.index], followSets[i], changed);
                            }
                        }
                    }
                }
            }       
        }
    }

    void Grammar::print() const
    {
        for(const auto &rule : mRules) {
            std::cout << "<" << rule.lhs << ">: ";
            for(unsigned int i=0; i<rule.rhs.size(); i++) {
                for(const auto &symbol : rule.rhs[i]) {
                    switch(symbol.type) {
                        case Symbol::Type::Terminal:
                            std::cout << mTerminals[symbol.index];
                            break;
                        case Symbol::Type::Nonterminal:
                            std::cout << "<" << mRules[symbol.index].lhs << ">";
                            break;
                        case Symbol::Type::Epsilon:
                            std::cout << "0";
                            break;
                    }
                    std::cout << " ";
                }
                if(i != rule.rhs.size() - 1) {
                    std::cout << "| ";
                }
            }
            std::cout << std::endl;
        }
    }
}