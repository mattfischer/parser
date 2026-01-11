pub struct Parser;

type Symbol = char;
type Range = (Symbol, Symbol);

pub enum Node {
    Symbol(Symbol),
    CharacterClass(Vec<Range>),
    Sequence(Vec<Node>),
    ZeroOrOne(Box<Node>),
    ZeroOrMore(Box<Node>),
    OneOrMore(Box<Node>),
    OneOf(Vec<Node>)
}

pub struct ParseError {
    message: String,
    pos: usize
}

impl ParseError {
    pub fn new(message: &str, pos: usize) -> ParseError {
        ParseError { message: message.to_string(), pos }
    }
}

impl Parser {
    pub fn parse(regex: &str) -> Result<Node, ParseError> {
        let mut pos = 0;
        let node = Self::parse_sequence(regex, &mut pos)?;
        if pos == regex.len() {
            return Ok(node);
        } else {
            let c = regex.chars().nth(pos).unwrap();
            return Err(ParseError::new(format!("Unexpected character {c}").as_str(), pos));
        }
    }

    fn parse_sequence(regex: &str, pos: &mut usize) -> Result<Node, ParseError> {
        let mut nodes = Vec::new();
        while *pos < regex.len() {
            match regex.chars().nth(*pos).unwrap() {
                '|' | ')' => break,
                _ => nodes.push(Self::parse_suffix(regex, pos)?)
            }
        }
        
        if nodes.len() == 1 {
            return Ok(nodes.pop().unwrap());
        } else {
            return Ok(Node::Sequence(nodes));
        }
    }

    fn parse_suffix(regex: &str, pos: &mut usize) -> Result<Node, ParseError> {
        let mut node = Self::parse_one_of(regex, pos)?;
        while *pos < regex.len() {
            let c = regex.chars().nth(*pos).unwrap();
            match c {
                '*' => node = Node::ZeroOrMore(Box::new(node)),
                '+' => node = Node::OneOrMore(Box::new(node)),
                '?' => node = Node::ZeroOrOne(Box::new(node)),
                _ => break
            }
            *pos += 1;
        }

        return Ok(node);
    }

    fn parse_one_of(regex: &str, pos: &mut usize) -> Result<Node, ParseError> {
        if regex.chars().nth(*pos).unwrap() == '(' {
            let mut nodes = Vec::new();
            *pos += 1;
            loop {
                nodes.push(Self::parse_sequence(regex, pos)?);
                match regex.chars().nth(*pos).unwrap_or_default() {
                    '|' => *pos += 1,
                    ')' => { *pos += 1; break; },
                    _ => return Err(ParseError::new("Expected | or )", *pos))
                }
            }

            if nodes.len() == 1 {
                return Ok(nodes.pop().unwrap());
            } else {
                return Ok(Node::OneOf(nodes));
            }
        } else {
            return Self::parse_symbol(regex, pos);
        }
    }

    fn parse_symbol(regex: &str, pos: &mut usize) -> Result<Node, ParseError> {
        if *pos >= regex.len() {
            return Err(ParseError::new("Expected symbol", *pos));
        }
        
        let c = regex.chars().nth(*pos).unwrap();
        match c {
            ')' | '|' | '*' | '+' | '?' => return Err(ParseError::new(format!("Unexpected character {c}").as_str(), *pos)),
            '[' => return Self::parse_character_class(regex, pos),
            '\\' => return Self::parse_escape(regex, pos),
            _ => {
                let symbol = c as Symbol;
                *pos += 1;

                return Ok(Node::Symbol(symbol));
            }
        }
    }

    fn parse_character_class(regex: &str, pos: &mut usize) -> Result<Node, ParseError> {
        if regex.chars().nth(*pos).unwrap() != '[' {
            return Err(ParseError::new("Expected [", *pos));
        }
        *pos += 1;
        
        if *pos >= regex.len() {
            return Err(ParseError::new("Expected ]", *pos));
        }

        let invert;
        if regex.chars().nth(*pos).unwrap() == '^' {
            invert = true;
            *pos += 1;
        } else {
            invert = false;
        }

        let mut ranges = Vec::new();
        loop {
            if *pos >= regex.len() {
                return Err(ParseError::new("Expected ]", *pos));
            }

            if regex.chars().nth(*pos).unwrap() == ']' {
                *pos += 1;
                break;
            }

            let start = regex.chars().nth(*pos).unwrap() as Symbol;
            let end;
            *pos += 1;
            
            if regex.chars().nth(*pos).unwrap_or_default() == '-' {
                *pos += 1;
                if *pos >= regex.len() {
                    return Err(ParseError::new("Expected symbol", *pos));
                }
                end = regex.chars().nth(*pos).unwrap() as Symbol;
                *pos += 1;
            } else {
                end = start;
            }

            ranges.push((start, end) as Range);
        }

        if invert {
            ranges = Self::invert_ranges(ranges);
        }

        return Ok(Node::CharacterClass(ranges));
    }

    fn invert_ranges(mut ranges: Vec<Range>) -> Vec<Range> {
        let mut output_ranges = Vec::new();
        ranges.sort_by_key(|a| a.0);
        let mut start = 0 as Symbol;
        for (range_start, range_end) in ranges {
            if range_start > start {
                output_ranges.push((start, (range_start as u8 - 1) as Symbol));
            }
            start = (range_end as u8 + 1) as Symbol;
        }
        output_ranges.push((start, 127 as Symbol));
        return output_ranges;
    }

    fn parse_escape(regex: &str, pos: &mut usize) -> Result<Node, ParseError> {
        if regex.chars().nth(*pos).unwrap() != '\\' {
            return Err(ParseError::new("Expected \\", *pos));
        }
        *pos += 1;

        if *pos >= regex.len() {
            return Err(ParseError::new("Incomplete escape", *pos));
        }
        
        let mut ranges = Vec::new();
        let mut invert = false;

        match regex.chars().nth(*pos).unwrap() {
            'S' => { invert = true; ranges.extend_from_slice(&[('\n', '\n'), (' ', ' '), ('\t', '\t')]); },
            's' => { ranges.extend_from_slice(&[(' ', ' '), ('\t', '\t')]); },
            'W' => { invert = true; ranges.extend_from_slice(&[('a', 'z'), ('A', 'Z'), ('0', '9'), ('_', '_')]); },
            'w' => { ranges.extend_from_slice(&[('a', 'z'), ('A', 'Z'), ('0', '9'), ('_', '_')]); },
            _ => ()
        }
        
        if ranges.len() > 0 {
            *pos += 1;
            if invert {
                ranges = Self::invert_ranges(ranges);
            }
            return Ok(Node::CharacterClass(ranges));
        }

        let c = regex.chars().nth(*pos).unwrap();
        let symbol = match c {
            't' => '\t',
            'n' => '\n',
            'r' => '\r',
            _ => c
        };
        *pos += 1;

        return Ok(Node::Symbol(symbol));
    }
}