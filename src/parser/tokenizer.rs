use crate::regex;
use regex::Matcher;

use std::io::BufRead;

type Value = usize;

pub const INVALID_TOKEN_VALUE : Value = usize::MAX;
pub const ERROR_TOKEN_VALUE : Value = INVALID_TOKEN_VALUE - 1;

pub struct Pattern {
    pub name : String,
    pub regex : String,
    pub value : Value
}

impl Pattern {
    pub fn new(name: &str, regex: &str, value: Value) -> Pattern {
        Pattern { name: name.to_string(), regex: regex.to_string(), value }
    }
}

type Configuration = Vec<Pattern>;

pub struct Tokenizer {
    configurations: Vec<Configuration>,
    matchers: Vec<Matcher>,
    end_value: Value,
    newline_value: Value
}

impl Tokenizer {
    pub fn new(configurations: Vec<Configuration>, end_value: Value, newline_value: Value) -> Tokenizer {
        let mut matchers = Vec::new();
        for configuration in &configurations {
            let mut patterns = Vec::new();
            for pattern in configuration {
                patterns.push(pattern.regex.to_string());
            }
            if let Ok(matcher) = Matcher::new(patterns) {
                matchers.push(matcher);
            }
        }

        return Tokenizer { configurations, matchers, end_value, newline_value };
    }
}

pub struct Token {
    pub value: Value,
    pub start: usize,
    pub line: usize,
    pub text: String
}

pub struct Stream {
    tokenizer: Tokenizer,
    input: Box<dyn BufRead>,
    current_line: String,
    consumed: usize,
    next_token: Token,
    line: usize,
    configuration: usize
}

impl Stream {
    pub fn new(tokenizer: Tokenizer, input: Box<dyn BufRead>) -> Stream {
        let next_token = Token { value: INVALID_TOKEN_VALUE, start: 0, line: 0, text: "".to_string() };

        return Stream { tokenizer, input, current_line: "".to_string(), consumed: 0, next_token, line: 0, configuration: 0 };
    }

    pub fn set_configuration(&mut self, configuration: usize) {
        if configuration < self.tokenizer.configurations.len() {
            self.configuration = configuration;
        }
    }

    pub fn next_token(&mut self) -> &Token {
        if self.line == 0 {
            self.consume_token();
        }

        return &self.next_token;
    }

    pub fn consume_token(&mut self) {
        if self.next_token.value == ERROR_TOKEN_VALUE || self.next_token.value == self.tokenizer.end_value {
            return;
        }

        let mut repeat = true;
        while repeat {
            while self.consumed >= self.current_line.len() {
                if self.configuration == self.current_line.len() && self.tokenizer.newline_value != INVALID_TOKEN_VALUE && self.line > 0 {
                    self.next_token = Token { value: self.tokenizer.newline_value, start: self.consumed, line: self.line, text: "<newline>".to_string() };
                    self.consumed += 1;
                    return;
                }

                self.current_line.clear();
                let num_read = self.input.read_line(&mut self.current_line);
                if num_read.is_err() || num_read.unwrap() == 0 {
                    self.next_token = Token { value: self.tokenizer.end_value, start: self.consumed, line: self.line, text: "<end>".to_string() };
                    return;
                } else {
                    self.current_line.pop();
                    self.consumed = 0;
                    self.line += 1;
                }
            }

            let (matched, pattern) = self.tokenizer.matchers[self.configuration].match_string(&self.current_line, self.consumed);
            if matched == 0 {
                self.next_token = Token { value: ERROR_TOKEN_VALUE, start: self.consumed, line: self.line, text: self.current_line[self.consumed..self.consumed+1].to_string() };
                repeat = false;
            } else {
                let value = self.tokenizer.configurations[self.configuration][pattern].value;
                if value != INVALID_TOKEN_VALUE {
                    self.next_token = Token { value, start: self.consumed, line: self.line, text: self.current_line[self.consumed..self.consumed+matched].to_string() };
                    repeat = false;
                }

                self.consumed += matched;
            }
        }
    }
}