#include "Parser.hpp"

#include <sstream>
#include <iostream>

namespace Regex {

    std::unique_ptr<Parser::Node> Parser::parseSymbol(const std::string &regex, int &pos)
    {
        if(pos >= regex.size()) {
            throw ParseException("Expected symbol", pos);
        }

        if(regex[pos] == ')' || regex[pos] == '|' || regex[pos] == '*' || regex[pos] == '+' || regex[pos] == '?') {
            std::stringstream ss;
            ss << "Unexpected character '" << regex[pos] << "'";
            throw ParseException(ss.str(), pos);
        }

        if(regex[pos] == '[') {
            return parseCharacterClass(regex, pos);
        } else {
            Symbol symbol = regex[pos];
            pos++;

            return std::make_unique<SymbolNode>(symbol);
        }
    }

    std::unique_ptr<Parser::Node> Parser::parseCharacterClass(const std::string &regex, int &pos)
    {
        if(regex[pos] != '[') {
            throw ParseException("Expected [", pos);
        }

        pos++;

        std::vector<CharacterClassNode::Range> ranges;
        while(true) {
            if(pos >= regex.size()) {
                throw ParseException("Expected ]", pos);
            }
            if(regex[pos] == ']') {
                pos++;
                break;
            }

            Symbol start = regex[pos];
            Symbol end = start;

            pos++;
            if(pos < regex.size() && regex[pos] == '-') {
                pos++;
                if(pos >= regex.size()) {
                    throw ParseException("Expected symbol", pos);
                }

                end = regex[pos];
                pos++;
            }
            ranges.push_back(CharacterClassNode::Range(start, end));
        }

        return std::make_unique<CharacterClassNode>(std::move(ranges));
    }

    std::unique_ptr<Parser::Node> Parser::parseOneOf(const std::string &regex, int &pos)
    {
        if(regex[pos] == '(') {
            std::vector<std::unique_ptr<Node>> nodes;
            pos++;
            while(true) {
                nodes.push_back(parseSequence(regex, pos));
                
                if(pos < regex.size()) { 
                    if(regex[pos] == ')') {
                        pos++;
                        break;
                    }
                    if(regex[pos] == '|') {
                        pos++;
                        continue;
                    }
                }
                
                throw ParseException("Expected | or )", pos);
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

    std::unique_ptr<Parser::Node> Parser::parseSuffix(const std::string &regex, int &pos)
    {
        std::unique_ptr<Node> node = parseOneOf(regex, pos);
        while(pos < regex.size()) {
            if(regex[pos] == '*') {
                node = std::make_unique<ZeroOrMoreNode>(std::move(node));
                pos++;
            } else if(regex[pos] == '+') {
                node = std::make_unique<OneOrMoreNode>(std::move(node));
                pos++;
            } else if(regex[pos] == '?') {
                node = std::make_unique<ZeroOrOneNode>(std::move(node));
                pos++;
            } else {
                break;
            }
        }

        return node;
    }

    std::unique_ptr<Parser::Node> Parser::parseSequence(const std::string &regex, int &pos)
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

    std::unique_ptr<Parser::Node> Parser::parse(const std::string &regex)
    {
        int pos = 0;
        std::unique_ptr<Node> node = parseSequence(regex, pos);
        if(pos != regex.size()) {
            std::stringstream ss;
            ss << "Unexpected character '" << regex[pos] << "'";
            throw ParseException(ss.str(), pos);
        }

        return node;
    }

    void Parser::Node::print(int depth) const
    {
        for(int i=0; i<depth; i++) std::cout << "  ";

        switch(type) {
            case Node::Type::Symbol:
                std::cout << "Symbol: " << static_cast<const SymbolNode*>(this)->symbol << std::endl;
                break;

            case Node::Type::CharacterClass:
                std::cout << "CharacterClass: ";

                for(const auto &r : static_cast<const CharacterClassNode*>(this)->ranges) {
                    Symbol start = r.first;
                    Symbol end = r.second;
                    if(start == end) {
                        std::cout << start << " ";
                    } else {
                        std::cout << start << "-" << end << " ";
                    }
                }
                std::cout << std::endl;
                break;
    
            case Node::Type::Sequence:
                std::cout << "Sequence:" << std::endl;
                for(const auto &n : static_cast<const SequenceNode*>(this)->nodes) {
                    n->print(depth + 1);
                }
                break;
            
            case Node::Type::ZeroOrOne:
                std::cout << "ZeroOrOne:" << std::endl;
                static_cast<const ZeroOrOneNode*>(this)->node->print(depth + 1);
                break;

            case Node::Type::ZeroOrMore:
                std::cout << "ZeroOrMore:" << std::endl;
                static_cast<const ZeroOrMoreNode*>(this)->node->print(depth + 1);
                break;

            case Node::Type::OneOrMore:
                std::cout << "OneOrMore:" << std::endl;
                static_cast<const OneOrMoreNode*>(this)->node->print(depth + 1);
                break;

            case Node::Type::OneOf:
                std::cout << "OneOf:" << std::endl;
                for(const auto &n : static_cast<const OneOfNode*>(this)->nodes) {
                    n->print(depth + 1);
                }
                break;
        }
    }
}