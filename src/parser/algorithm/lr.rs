mod lalr;
mod slr;
mod parse_table;

use crate::parser;
use parser::Grammar;
use parser::grammar::Symbol;
use parser::tokenizer::Stream;

use parse_table::{Conflict, Lookahead, ParseTable, ParseTableEntry};

use slr::LookaheadSLR;
use lalr::LookaheadLALR;

use std::collections::HashMap;

type ParseItem<T> = (usize, Option<T>);
type TerminalDecorator<T> = dyn Fn(&parser::tokenizer::Token) -> T;
type Reducer<T> = dyn Fn(&[ParseItem<T>]) -> T;

type ParseStack<T> = Vec<ParseItem<T>>;

pub struct LR<T> {
    pub grammar: Grammar,
    parse_table: ParseTable,
    terminal_decorators: HashMap<usize, Box<TerminalDecorator<T>>>,
    reducers: HashMap<usize, Box<Reducer<T>>>
}

#[allow(dead_code)]
impl<T> LR<T> {
    pub fn new_slr(grammar: Grammar) -> Result<LR<T>, Conflict> {
        let parse_table = ParseTable::new::<LookaheadSLR>(&grammar)?;
        let lr = LR { grammar, parse_table, terminal_decorators: HashMap::new(), reducers: HashMap::new() };

        return Ok(lr);
    }

    pub fn new_lalr(grammar: Grammar) -> Result<LR<T>, Conflict> {
        let parse_table = ParseTable::new::<LookaheadLALR>(&grammar)?;
        let lr = LR { grammar, parse_table, terminal_decorators: HashMap::new(), reducers: HashMap::new() };

        return Ok(lr);
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

        let mut state_stack = Vec::new();
        let mut state = 0;

        while !self.parse_table.is_accept(state) {
            state_stack.push((state, parse_stack.len()));
            match self.parse_table.action_for_terminal(state, stream.next_token().value) {
                ParseTableEntry::Shift(next_state) => {
                    self.shift(stream.next_token(), &mut parse_stack);
                    stream.consume_token();
                    state = *next_state;
                },
                ParseTableEntry::Reduce((rule, rhs)) => {
                    let rule_rhs = &self.grammar.rules[*rule].rhs[*rhs];
                    for symbol in rule_rhs {
                        match symbol {
                            Symbol::Epsilon => (),
                            _ => {
                                state_stack.pop();
                            }
                        }
                    }

                    let (back_state, parse_stack_start) = state_stack[state_stack.len() - 1];
                    self.reduce(*rule, parse_stack_start, &mut parse_stack);

                    if let ParseTableEntry::Shift(next_state) = self.parse_table.action_for_rule(back_state, *rule) {
                        state = *next_state;
                    } else {
                        return None;
                    }
                },
                ParseTableEntry::Error => {
                    return None;
                }
            }
        }

        self.reduce(self.grammar.start_rule, 0, &mut parse_stack);

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

    fn reduce(&self, rule: usize, stack_start: usize, parse_stack: &mut ParseStack<T>) {
        if let Some(reducer) = self.reducers.get(&rule) {
            let data = reducer(&parse_stack[stack_start..]);
            parse_stack.drain(stack_start..);

            let parse_item = (rule, Some(data));
            parse_stack.push(parse_item);
        }
    }
}