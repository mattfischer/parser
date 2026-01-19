use crate::parser;
use parser::ExtendedGrammar;
use parser::Tokenizer;

use std::io::BufRead;
use std::collections::HashMap;

pub struct DefReader {
    stream: parser::tokenizer::Stream,
    patterns: Vec<parser::tokenizer::Pattern>,
    rules: Vec<parser::extended_grammar::Rule>,
    anonymous_terminals: HashMap<String, usize>
}

pub struct ParseError {
    pub message: String,
    pub line: usize
}

impl ParseError {
    pub fn new(message: &str, line: usize) -> ParseError {
        ParseError { message: message.to_string(), line }
    }
}

#[derive(Copy, Clone)]
enum Token {
    Epsilon,
    Terminal,
    Nonterminal,
    Colon,
    Pipe,
    LParen,
    RParen,
    Star,
    Plus,
    Question,
    Literal,
    Regex,
    Newline,
    End
}

impl DefReader {
    pub fn parse(input: Box<dyn BufRead>) -> Result<(Tokenizer, ExtendedGrammar), ParseError> {
        let def_tokenizer = Self::make_def_tokenizer();

        let stream = parser::tokenizer::Stream::new(def_tokenizer, input);
        let mut def_reader = DefReader { stream, patterns: Vec::new(), rules: Vec::new(), anonymous_terminals: HashMap::new() };

        def_reader.parse_grammar()?;

        let mut terminals = Vec::new();
        for (i, pattern) in def_reader.patterns.iter_mut().enumerate() {
            if pattern.name == "IGNORE" {
                pattern.value = parser::tokenizer::INVALID_TOKEN_VALUE;
            } else {
                pattern.value = i;
            }

            terminals.push(pattern.name.clone());
        }
        let end_value = terminals.len();
        terminals.push(String::from("END"));

        let start_rule;
        if let Some(rule) = def_reader.rules.iter().position(|x| x.lhs == "root") {
            start_rule = rule;
        } else {
            return Err(ParseError::new("No <root> nonterminal defined", 0));
        }

        let configurations = vec![def_reader.patterns];
        let tokenizer = Tokenizer::new(configurations, end_value, parser::tokenizer::INVALID_TOKEN_VALUE);
        let extended_grammar = ExtendedGrammar::new(terminals, def_reader.rules, start_rule);
        return Ok((tokenizer, extended_grammar));
    }

    fn error_expected(&self, name: &str) -> ParseError {
        return ParseError::new(&format!("Expected: {name}"), self.stream.line);
    }

    fn error_expected_token(&self, token: Token) -> ParseError {
        let name  = match token {
            Token::Newline => "<newline>",
            _ => self.stream.pattern_name(token as usize)
        };

        return self.error_expected(name);
    }

    fn match_token_with_configuration(&mut self, token: Token, new_configuration: usize) -> Option<String> {
        let result;
        if self.stream.next_token().value == token as usize {
            result = Some(self.stream.next_token().text.clone());
            self.stream.set_configuration(new_configuration);
            self.stream.consume_token();
        } else {
            result = None;
        }
        return result;
    }

    fn match_token(&mut self, token: Token) -> Option<String> {
        return self.match_token_with_configuration(token, self.stream.configuration);
    }

    fn expect_token_with_configuration(&mut self, token: Token, new_configuration: usize) -> Result<String, ParseError> {
        if let Some(text) = self.match_token_with_configuration(token, new_configuration) {
            return Ok(text)
        } else {
            return Err(self.error_expected_token(token));
        }
    }

    fn expect_token(&mut self, token: Token) -> Result<String, ParseError> {
        return self.expect_token_with_configuration(token, self.stream.configuration);
    }

    fn parse_grammar(&mut self) -> Result<(), ParseError> {
        while !self.stream.is_end() {
            if let Some(pattern) = self.try_parse_pattern()? {
                if let Some(index) = self.patterns.iter().position(|x| x.name == pattern.name) {
                    self.patterns[index] = pattern;
                } else {
                    self.patterns.push(pattern);
                }
            } else if let Some(rule) = self.try_parse_rule()? {
                if let Some(index) = self.rules.iter().position(|x| x.lhs == rule.lhs) {
                    self.rules[index] = rule;
                } else {
                    self.rules.push(rule);
                }
            } else if let Some(_) = self.match_token(Token::Newline) {
                continue;
            } else {
                return Err(self.error_expected("item"));
            }
        }

        return Ok(());
    }

    fn try_parse_pattern(&mut self) -> Result<Option<parser::tokenizer::Pattern>, ParseError> {
        if let Some(text) = self.match_token(Token::Terminal) {
            self.expect_token_with_configuration(Token::Colon, 1)?;
            let regex = self.expect_token(Token::Regex)?;
            self.expect_token_with_configuration(Token::Newline, 0)?;
            return Ok(Some(parser::tokenizer::Pattern::new(&text, &regex, 0)));
        } else {
            return Ok(None);
        }
    }

    fn try_parse_rule(&mut self) -> Result<Option<parser::extended_grammar::Rule>, ParseError> {
        if let Some(text) = self.match_token(Token::Nonterminal) {
            self.expect_token(Token::Colon)?;
            let rhs = self.parse_rhs_options()?;
            self.expect_token(Token::Newline)?;
            let lhs = text[1..text.len() - 1].to_string();
            let rule = parser::extended_grammar::Rule { lhs, rhs };
            return Ok(Some(rule));
        } else {
            return Ok(None);
        }
    }

    fn parse_rhs_options(&mut self) -> Result<parser::extended_grammar::RHSNode, ParseError> {
        let mut options = Vec::new();
        loop { 
            options.push(self.parse_rhs_sequence()?);
            if let Some(_) = self.match_token(Token::Pipe) {
                continue;
            } else {
                break;
            }
        }
        let rhs_node;
        if options.len() == 1 {
            rhs_node = options.pop().unwrap();
        } else {
            rhs_node = parser::extended_grammar::RHSNode::OneOf(options);
        }

        return Ok(rhs_node);
    }

    fn parse_rhs_sequence(&mut self) -> Result<parser::extended_grammar::RHSNode, ParseError> {
        let mut items = Vec::new();

        while let Some(item) = self.try_parse_rhs_item()? {
            items.push(item);
        }

        if items.len() == 1 {
            return Ok(items.pop().unwrap());
        } else {
            return Ok(parser::extended_grammar::RHSNode::Sequence(items));
        }
    }

    pub fn try_parse_rhs_item(&mut self) -> Result<Option<parser::extended_grammar::RHSNode>, ParseError> {
        if let Some(symbol) = self.try_parse_rhs_symbol()? {
            let mut rhs_item = symbol;
            loop {
                if let Some(_) = self.match_token(Token::Star) {
                    rhs_item = parser::extended_grammar::RHSNode::ZeroOrMore(Box::new(rhs_item));
                } else if let Some(_) = self.match_token(Token::Plus) {
                    rhs_item = parser::extended_grammar::RHSNode::OneOrMore(Box::new(rhs_item));
                } else if let Some(_) = self.match_token(Token::Question) {
                    rhs_item = parser::extended_grammar::RHSNode::ZeroOrOne(Box::new(rhs_item));
                } else {
                    break;
                }
            }
            return Ok(Some(rhs_item));
        } else {
            return Ok(None);
        }
    }

    fn try_parse_rhs_symbol(&mut self) -> Result<Option<parser::extended_grammar::RHSNode>, ParseError> {
        if let Some(text) = self.match_token(Token::Terminal) {
            let pattern_name = text;
            let index;
            if let Some(i) = self.patterns.iter().position(|x| x.name == pattern_name) {
                index = i;
            } else {
                index = self.patterns.len();
                let dummy_pattern = parser::tokenizer::Pattern::new(&pattern_name, "", 0);
                self.patterns.push(dummy_pattern);
            }
            let symbol = parser::extended_grammar::Symbol::Terminal(index);
            return Ok(Some(parser::extended_grammar::RHSNode::Symbol(symbol)));
        } else if let Some(text) = self.match_token(Token::Nonterminal) {
            let rule_lhs = &text[1..text.len() - 1];
            let index;
            if let Some(i) = self.rules.iter().position(|x| x.lhs == rule_lhs) {
                index = i;
            } else {
                index = self.rules.len();
                let dummy_symbol = parser::extended_grammar::Symbol::Terminal(0);
                let dummy_rule = parser::extended_grammar::Rule { lhs: rule_lhs.to_string(), rhs: parser::extended_grammar::RHSNode::Symbol(dummy_symbol) };
                self.rules.push(dummy_rule);
            }
            let symbol = parser::extended_grammar::Symbol::Nonterminal(index);
            return Ok(Some(parser::extended_grammar::RHSNode::Symbol(symbol)));
        } else if let Some(text) = self.match_token(Token::Literal) {
            let literal_text = &text[1..text.len() - 1];

            let index;
            if self.anonymous_terminals.contains_key(literal_text) {
                index = self.anonymous_terminals[literal_text];
            } else {
                index = self.patterns.len();
                let pattern_name = format!("'{literal_text}'");
                let escaped_text = self.escape(&literal_text);
                let pattern = parser::tokenizer::Pattern::new(&pattern_name, &escaped_text, 0);
                self.patterns.push(pattern);
                self.anonymous_terminals.insert(literal_text.to_string(), index);
            }

            let symbol = parser::extended_grammar::Symbol::Terminal(index);
            return Ok(Some(parser::extended_grammar::RHSNode::Symbol(symbol)));
        } else if let Some(_) = self.match_token(Token::LParen) {
            let rhs_list = self.parse_rhs_options()?;
            self.expect_token(Token::RParen)?;
            return Ok(Some(rhs_list));
        } else {
            return Ok(None);
        }
    }

    fn make_def_tokenizer() -> Tokenizer {
        let pattern = |regex: &str, name: &str, value: Token| {
            parser::tokenizer::Pattern { regex: regex.to_string(), name: name.to_string(), value: value as usize}
        };

        let pattern_ignore = |regex: &str, name: &str| {
            parser::tokenizer::Pattern { regex: regex.to_string(), name: name.to_string(), value: parser::tokenizer::INVALID_TOKEN_VALUE}
        };

        let configurations = vec![
            vec![
                pattern("0", "<epsilon>", Token::Epsilon),
                pattern("\\w+", "<terminal>", Token::Terminal),
                pattern("<\\w+>", "<nonterminal>", Token::Nonterminal),
                pattern(":", ":", Token::Colon),
                pattern("\\|", "|", Token::Pipe),
                pattern("\\(", "(", Token::LParen),
                pattern("\\)", ")", Token::RParen),
                pattern("\\+", "+", Token::Plus),
                pattern("\\*", "*", Token::Star),
                pattern("\\?", "?", Token::Question),
                pattern("'[^']+'", "<literal>", Token::Literal),
                pattern_ignore("\\s", "<whitespace>")  
            ],
            vec![
                pattern("\\S+", "<regex>", Token::Regex),
                pattern_ignore("\\s", "<whitespace>")
            ]
        ];

        return Tokenizer::new(configurations, Token::End as usize, Token::Newline as usize);
    }

    fn escape(&self, input: &str) -> String {
        let mut result = String::new();
        for c in input.chars() {
            match c {
                ' ' => result += "\\s",
                '+' | '*' | '?' | '(' | ')' | '[' | ']' => { result.push('\\'); result.push(c); },
                _ => result.push(c)
            }
        }

        return result;
    }
}