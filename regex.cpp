#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <exception>

namespace Parser {
    class ParseException : public std::exception {
    };

    typedef int Symbol;

    struct Node {
        enum class Type {
            Symbol,
            Sequence,
            ZeroOrMore,
            OneOrMore,
            OneOf
        };
        Type type;
    };

    struct SymbolNode : public Node {
        SymbolNode(Symbol s) { type = Type::Symbol; symbol = s; }

        Symbol symbol;
    };

    struct SequenceNode : public Node {
        SequenceNode(std::vector<std::unique_ptr<Node>> &n) { type = Type::Sequence; nodes = std::move(n); }
    
        std::vector<std::unique_ptr<Node>> nodes;
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
        OneOfNode(std::vector<std::unique_ptr<Node>> &n) { type = Type::OneOf; nodes = std::move(n); }
    
        std::vector<std::unique_ptr<Node>> nodes;
    };

    std::unique_ptr<Node> parseSymbol(const std::string &regex, int &pos)
    {
        Symbol symbol = regex[pos];
        pos++;

        return std::make_unique<SymbolNode>(symbol);
    }

    std::unique_ptr<Node> parseSequence(const std::string &regex, int &pos);
 
    std::unique_ptr<Node> parseOneOf(const std::string &regex, int &pos)
    {
        if(regex[pos] == '(') {
            std::vector<std::unique_ptr<Node>> nodes;
            pos++;
            while(true) {
                if(pos >= regex.size()) {
                    throw ParseException();
                }
                nodes.push_back(parseSequence(regex, pos));
                
                if(pos >= regex.size()) {
                    throw ParseException();
                }
                if(regex[pos] == ')') {
                    pos++;
                    break;
                }
                if(regex[pos] == '|') {
                    pos++;
                    continue;
                }
                throw ParseException();
            }
            if(nodes.size() == 1) {
                return std::move(nodes[0]);
            } else {
                return std::make_unique<OneOfNode>(std::move(nodes));
            }
        } else {
            return parseSymbol(regex, pos);
        }
    }

    std::unique_ptr<Node> parseSuffix(const std::string &regex, int &pos)
    {
        std::unique_ptr<Node> node = parseOneOf(regex, pos);
        while(pos < regex.size()) {
            if(regex[pos] == '*') {
                node = std::make_unique<ZeroOrMoreNode>(std::move(node));
                pos++;
            } else if(regex[pos] == '+') {
                node = std::make_unique<OneOrMoreNode>(std::move(node));
                pos++;
            } else {
                break;
            }
        }

        return node;
    }

    std::unique_ptr<Node> parseSequence(const std::string &regex, int &pos)
    {
        std::vector<std::unique_ptr<Node>> nodes;
        while(pos < regex.size() && regex[pos] != '|' && regex[pos] != ')') {
            nodes.push_back(parseSuffix(regex, pos));
        }

        if(nodes.size() == 1) {
            return std::move(nodes[0]);
        } else {
            return std::make_unique<SequenceNode>(std::move(nodes));
        }
    }

    std::unique_ptr<Node> parse(const std::string &regex)
    {
        int pos = 0;
        return parseSequence(regex, pos);
    }

    void printNode(Node &node, int depth = 0)
    {
        for(int i=0; i<depth; i++) std::cout << "  ";

        switch(node.type) {
            case Node::Type::Symbol:
                std::cout << "Symbol: " << char(static_cast<SymbolNode&>(node).symbol) << std::endl;
                break;
            
            case Node::Type::Sequence:
                std::cout << "Sequence:" << std::endl;
                for(const auto &n : static_cast<SequenceNode&>(node).nodes) {
                    printNode(*n, depth + 1);
                }
                break;
            
            case Node::Type::ZeroOrMore:
                std::cout << "ZeroOrMore:" << std::endl;
                printNode(*static_cast<ZeroOrMoreNode&>(node).node, depth + 1);
                break;

            case Node::Type::OneOrMore:
                std::cout << "OneOrMore:" << std::endl;
                printNode(*static_cast<OneOrMoreNode&>(node).node, depth + 1);
                break;

            case Node::Type::OneOf:
                std::cout << "OneOf:" << std::endl;
                for(const auto &n : static_cast<OneOfNode&>(node).nodes) {
                    printNode(*n, depth + 1);
                }
                break;
        }
    }
}

int main(int argc, char *argv[])
{
    std::string input = "a(b|c(de)*)f+";
    std::unique_ptr<Parser::Node> node = Parser::parse(input);
    Parser::printNode(*node);

    return 0;
}