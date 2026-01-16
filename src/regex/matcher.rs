use crate::regex;
use regex::DFA;
use regex::Encoding;
use regex::NFA;
use regex::Parser;

pub struct ParseError {
    pub pattern: usize,
    pub pos: usize,
    pub message: String
}

impl ParseError {
    pub fn new(pattern: usize, pos: usize, message: String) -> ParseError {
        ParseError { pattern, pos, message }
    }
}
pub struct Matcher {
    dfa: DFA,
    encoding: Encoding
}

impl Matcher {
    pub fn new(patterns: Vec<String>) -> Result<Matcher, ParseError> {
        let mut nodes = Vec::new();
        for (i, pattern) in patterns.iter().enumerate() {
            match Parser::parse(pattern.as_str()) {
                Ok(node) => nodes.push(node),
                Err(error) => return Err(ParseError::new(i, error.pos, error.message))
            } 
        }

        println!("*** Parse nodes ***");
        for node in &nodes {
            node.print(0);
        }
        println!();

        let encoding = Encoding::new(&nodes);
        println!("*** Encoding ***");
        encoding.print();
        println!();

        let nfa = NFA::new(&nodes, &encoding);
        println!("*** NFA ***");
        nfa.print();
        println!();

        let dfa = DFA::new(&nfa, &encoding);
        println!("*** DFA ***");
        dfa.print();
        println!();

        return Ok(Matcher { dfa, encoding });
    }

    pub fn match_string(&self, string: &str, start: usize) -> (usize, usize) {
        let mut state = self.dfa.start_state;
        let mut num_matched = 0;
        let mut matched_pattern = usize::MAX;

        for i in start..string.len() {
            let code_point = self.encoding.code_point(string.chars().nth(i).unwrap());
            if code_point == regex::encoding::INVALID_CODE_POINT {
                break;
            }
            let next_state = self.dfa.transition(state, code_point);
            if next_state == self.dfa.reject_state {
                break;
            } else {
                if let Some(pattern) = self.dfa.accept(next_state) {
                    num_matched = (i - start) + 1;
                    matched_pattern = pattern;
                }
                state = next_state;
            }
        }

        return (num_matched, matched_pattern);
    }
}