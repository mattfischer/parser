#ifndef DEFREADER_HPP
#define DEFREADER_HPP

#include "Regex/Matcher.hpp"
#include "Parser/Grammar.hpp"
#include "Parser/ExtendedGrammar.hpp"
#include "Tokenizer.hpp"

#include <string>
#include <vector>
#include <map>
#include <memory>

class DefReader {
public:
    DefReader(const std::string &filename);

    bool valid() const;

    const Tokenizer &tokenizer() const;
    const Parser::Grammar &grammar() const;

    struct ParseError {
        unsigned int line;
        std::string message;
    };
    const ParseError &parseError() const;

private:
    typedef std::vector<std::vector<std::string>> Rule;

    struct DefNode {
        enum class Type {
            List,
            Terminal,
            Nonterminal,
            Literal,
            Regex,
            Pattern,
            Rule,
            RhsSequence,
            RhsOneOf,
            RhsZeroOrMore,
            RhsOneOrMore,
            RhsZeroOrOne
        };

        template<typename ...Children> DefNode(Type t, std::unique_ptr<Children>... c) : type(t) {
            children.reserve(sizeof...(c));
            (children.push_back(std::move(c)), ...);
        }
        DefNode(Type t, const std::string &s, unsigned int l) : type(t), string(s), line(l) {}

        Type type;
        std::vector<std::unique_ptr<DefNode>> children;
        std::string string;
        unsigned int line;
    };

    std::unique_ptr<DefNode> parseFile(const std::string &filename);
    void createDefGrammar();
    std::unique_ptr<Parser::ExtendedGrammar::RhsNode> createRhsNode(const DefNode &defNode);

    std::unique_ptr<Tokenizer> mDefTokenizer;
    std::unique_ptr<Parser::Grammar> mDefGrammar;
    
    std::vector<std::string> mTerminals;
    std::map<std::string, unsigned int> mTerminalMap;
    std::map<std::string, unsigned int> mAnonymousTerminalMap;
    std::vector<std::string> mTerminalNames;

    std::vector<Parser::ExtendedGrammar::Rule> mRules;
    std::map<std::string, unsigned int> mRuleMap;

    std::unique_ptr<Tokenizer> mTokenizer;
    std::unique_ptr<Parser::Grammar> mGrammar;
    ParseError mParseError;
};
#endif