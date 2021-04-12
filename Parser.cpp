#include "Parser.hpp"

#include <sstream>
#include <iostream>

std::unique_ptr<Parser::Node> Parser::parseSymbol(const std::string &regex, int &pos)
{
    if(pos >= regex.size()) {
        throw ParseException("Expected symbol", pos);
    }

    if(regex[pos] == ')' || regex[pos] == '|' || regex[pos] == '*' || regex[pos] == '+') {
        std::stringstream ss;
        ss << "Unexpected character '" << regex[pos] << "'";
        throw ParseException(ss.str(), pos);
    }

    Symbol symbol = regex[pos];
    pos++;

    return std::make_unique<SymbolNode>(symbol);
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

void Parser::printNode(Node &node, int depth)
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
