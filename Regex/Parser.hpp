#ifndef REGEX_PARSER_HPP
#define REGEX_PARSER_HPP

#include <exception>
#include <string>
#include <vector>
#include <memory>
#include <tuple>

namespace Regex {

    class Parser {
    public:
        class ParseException : public std::exception {
        public:
            ParseException(const std::string &m, int p) { message = m; pos = p; }

            std::string message;
            int pos;
        };

        typedef char Symbol;

        struct Node {
            enum class Type {
                Symbol,
                CharacterClass,
                Sequence,
                ZeroOrOne,
                ZeroOrMore,
                OneOrMore,
                OneOf
            };
            Type type;

            void print(int depth = 0) const;
        };

        struct SymbolNode : public Node {
            SymbolNode(Symbol s) { type = Type::Symbol; symbol = s; }

            Symbol symbol;
        };

        struct CharacterClassNode : public Node {
            typedef std::pair<Symbol, Symbol> Range;

            CharacterClassNode(std::vector<Range> &&r) { type = Type::CharacterClass; ranges = std::move(r); }

            std::vector<Range> ranges;
        };

        struct SequenceNode : public Node {
            SequenceNode(std::vector<std::unique_ptr<Node>> &n) { type = Type::Sequence; nodes = std::move(n); }
        
            std::vector<std::unique_ptr<Node>> nodes;
        };

        struct ZeroOrOneNode : public Node {
            ZeroOrOneNode(std::unique_ptr<Node> &n) { type = Type::ZeroOrOne; node = std::move(n); }

            std::unique_ptr<Node> node;
        };

        struct ZeroOrMoreNode : public Node {
            ZeroOrMoreNode(std::unique_ptr<Node> &n) { type = Type::ZeroOrMore; node = std::move(n); }
        
            std::unique_ptr<Node> node;
        };

        struct OneOrMoreNode : public Node {
            OneOrMoreNode(std::unique_ptr<Node> &n) { type = Type::OneOrMore; node = std::move(n); }
        
            std::unique_ptr<Node> node;
        };

        struct OneOfNode : public Node {
            OneOfNode(std::vector<std::unique_ptr<Node>> &&n) { type = Type::OneOf; nodes = std::move(n); }
        
            std::vector<std::unique_ptr<Node>> nodes;
        };

        static std::unique_ptr<Node> parse(const std::string &regex);
    
    private:
        static std::unique_ptr<Node> parseSymbol(const std::string &regex, int &pos);
        static std::unique_ptr<Node> parseCharacterClass(const std::string &regex, int &pos);
        static std::unique_ptr<Node> parseEscape(const std::string &regex, int &pos);
        static std::unique_ptr<Node> parseSequence(const std::string &regex, int &pos);
        static std::unique_ptr<Node> parseOneOf(const std::string &regex, int &pos);
        static std::unique_ptr<Node> parseSuffix(const std::string &regex, int &pos);
    };
}

#endif