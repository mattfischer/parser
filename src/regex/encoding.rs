use crate::regex;
use regex::parser::Node;

pub type InputSymbol = char;
pub type InputSymbolRange = (InputSymbol, InputSymbol);
pub type CodePoint = usize;
pub const INVALID_CODE_POINT: usize = usize::MAX;

use std::collections::VecDeque;

pub struct Encoding {
    input_symbol_ranges: Vec<InputSymbolRange>,
    total_range: InputSymbolRange,
    symbol_map: Vec<CodePoint>
}

impl Encoding {
    pub fn new(nodes: &Vec<Node>) -> Encoding {
        let mut node_symbol_ranges = VecDeque::new();
        
        for node in nodes {
            Self::visit_node(node, &mut node_symbol_ranges);
        }
        node_symbol_ranges.make_contiguous().sort_by_key(|a| a.0);

        let mut input_symbol_ranges = Vec::new();
        let mut current = node_symbol_ranges.pop_front().unwrap();
        while node_symbol_ranges.len() > 0 {
            let next = node_symbol_ranges.pop_front().unwrap();

            if current.1 < next.0 {
                input_symbol_ranges.push(current);
                current = next;
                continue;
            }

            if next.0 > current.0 {
                let head = (current.0, (next.0 as u8 - 1) as char);
                input_symbol_ranges.push(head);
                current.0 = next.0;
            }

            if current.1 != next.1 {
                let rest = ((u8::min(current.1 as u8, next.1 as u8) + 1) as char, u8::max(current.1 as u8, next.1 as u8) as char);
                let index = node_symbol_ranges.partition_point(|a| a.0 <= rest.0);
                node_symbol_ranges.insert(index, rest);
            }

            current.1 = u8::min(current.1 as u8, next.1 as u8) as char;
        }

        input_symbol_ranges.push(current);
        let total_range = (input_symbol_ranges.first().unwrap().0, input_symbol_ranges.last().unwrap().1);

        let mut symbol_map = vec![INVALID_CODE_POINT; total_range.1 as usize - total_range.0 as usize + 1];
        for (i, range) in input_symbol_ranges.iter().enumerate() {
            for j in range.0..=range.1 {
                symbol_map[(j as usize) - (total_range.0 as usize)] = i;
            }
        }

        return Encoding { input_symbol_ranges, total_range, symbol_map };
    }

    pub fn num_code_points(&self) -> usize {
        return self.input_symbol_ranges.len();
    }

    fn visit_node(node: &Node, input_symbol_ranges: &mut VecDeque<InputSymbolRange>) {
        match node {
            Node::Symbol(symbol) => input_symbol_ranges.push_back((*symbol, *symbol)),
            Node::CharacterClass(ranges) => for (start, end) in ranges { input_symbol_ranges.push_back((*start, *end)); }
            Node::OneOf(nodes) => for node in nodes { Self::visit_node(node, input_symbol_ranges); },
            Node::ZeroOrOne(node) => Self::visit_node(node, input_symbol_ranges),
            Node::ZeroOrMore(node) => Self::visit_node(node, input_symbol_ranges),
            Node::OneOrMore(node) => Self::visit_node(node, input_symbol_ranges),
            Node::Sequence(nodes) => for node in nodes { Self::visit_node(node, input_symbol_ranges); }
        }
    }

    pub fn code_point_ranges(&self, mut input_symbol_range: InputSymbolRange) -> Vec<CodePoint> {
        let mut code_points = Vec::new();
        while input_symbol_range.0 >= 0 as char && input_symbol_range.0 <= input_symbol_range.1 {
            let index = match self.input_symbol_ranges.binary_search_by_key(&input_symbol_range.0, |a| a.0) {
                Ok(i) => i,
                Err(i) => i
            };
            code_points.push(index as CodePoint);
            input_symbol_range.0 = (self.input_symbol_ranges[index].1 as u8 + 1) as char;
        }

        return code_points;
    }

    pub fn code_point(&self, symbol: InputSymbol) -> CodePoint {
        if symbol < self.total_range.0 || symbol > self.total_range.1 {
            return INVALID_CODE_POINT;
        }

        return self.symbol_map[symbol as usize - self.total_range.0 as usize];
    }

    pub fn print(&self) {
        for (i, (first, last)) in self.input_symbol_ranges.iter().enumerate() {
            if first == last {
                println!("{i}: {first}");
            } else {
                println!("{i}: {first}-{last}");
            }
        }
    }
}