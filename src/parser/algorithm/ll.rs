mod parse_table;
use parse_table::{Conflict, ParseTable};

use crate::parser;
use parser::Grammar;
use parser::tokenizer::Stream;

use std::collections::HashMap;

type ParseItem<T> = (usize, Option<T>);
type TerminalDecorator<T> = dyn Fn(&parser::tokenizer::Token) -> T;
type Reducer<T> = dyn Fn(&[ParseItem<T>]) -> T;

pub struct LL<T> {
    pub grammar: Grammar,
    parse_table: ParseTable,
    terminal_decorators: HashMap<usize, Box<TerminalDecorator<T>>>,
    reducers: HashMap<usize, Box<Reducer<T>>>
}

type ParseStack<T> = Vec<ParseItem<T>>;

enum PredictItem {
    Terminal(usize),
    Nonterminal(usize),
    Reduce(usize, usize)
}

#[allow(dead_code)]
impl<T> LL<T> {
    pub fn new(grammar: Grammar) -> Result<LL<T>, Conflict> {
        let parse_table= ParseTable::new(&grammar)?;
        let ll = Self { grammar, parse_table, terminal_decorators: HashMap::new(), reducers: HashMap::new() };
        return Ok(ll);
    }

    pub fn add_terminal_decorator<F>(&mut self, terminal: &str, terminal_decorator: F)
    where F: 'static + Fn(&parser::tokenizer::Token) -> T {
        if let Some(idx) = self.grammar.terminal_index(terminal) {
            self.terminal_decorators.insert(idx, Box::new(terminal_decorator));
        }
    }

    pub fn add_reducer<F>(&mut self, rule: &str, reducer: F) 
    where F: 'static + Fn(&[ParseItem<T>]) -> T {
        if let Some(idx) = self.grammar.rule_index(rule) {        
            self.reducers.insert(idx, Box::new(reducer));
        }
    }

    pub fn parse(&self, mut stream: Stream) -> Option<T> {
        let mut parse_stack: ParseStack<T> = ParseStack::new();
        let mut predict_stack = Vec::new();

        predict_stack.push(PredictItem::Nonterminal(self.grammar.start_rule));

        while let Some(predict_item) = predict_stack.pop() {
            match predict_item {
                PredictItem::Terminal(terminal) => {
                    if stream.next_token().value == terminal {
                        self.shift(stream.next_token(), &mut parse_stack);

                        stream.consume_token();
                    } else {
                        return None;
                    }
                },
                PredictItem::Nonterminal(rule) => {
                    let next_rule = rule;
                    let next_rhs = self.parse_table.rhs(rule, stream.next_token().value);

                    if next_rhs == usize::MAX {
                        return None;
                    }

                    if self.can_reduce(next_rule) {
                        predict_stack.push(PredictItem::Reduce(next_rule, parse_stack.len()));
                    }

                    let symbols = &self.grammar.rules[next_rule].rhs[next_rhs];
                    for i in 0..symbols.len() {
                        let ri = symbols.len() - 1 - i;
                        let s = &symbols[ri];
                        match s {
                            parser::grammar::Symbol::Terminal(terminal_index) => predict_stack.push(PredictItem::Terminal(*terminal_index)),
                            parser::grammar::Symbol::Nonterminal(rule_index) => predict_stack.push(PredictItem::Nonterminal(*rule_index)),
                            parser::grammar::Symbol::Epsilon => ()
                        }
                    }
                },
                PredictItem::Reduce(current_rule, parse_stack_start) => {
                    self.reduce(current_rule, parse_stack_start, &mut parse_stack);
                }
            }
        }

        if let Some((_, Some(data))) = parse_stack.pop() {
            return Some(data);
        } else {
            return None;
        }
    }

    fn shift(&self, token: &parser::tokenizer::Token, parse_stack: &mut ParseStack<T>) {
        let data;
        if let Some(terminal_decorator) = self.terminal_decorators.get(&token.value) {
            data = Some(terminal_decorator(token));
        } else {
            data = None;
        }

        let parse_item = (token.value, data);
        parse_stack.push(parse_item);
    }

    fn can_reduce(&self, rule: usize) -> bool {
        return self.reducers.contains_key(&rule);
    }

    fn reduce(&self, rule: usize, stack_start: usize, parse_stack: &mut ParseStack<T>) {
        if let Some(reducer) = self.reducers.get(&rule) {
            let data = reducer(&parse_stack[stack_start..]);
            parse_stack.drain(stack_start..);

            let parse_item = (rule, Some(data));
            parse_stack.push(parse_item);
        }
    }
}
