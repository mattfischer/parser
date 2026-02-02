use crate::parser;
use parser::Grammar;
use parser::tokenizer::Stream;

use crate::util;
use util::Table;

use std::collections::HashMap;
use std::collections::HashSet;

type ParseItem<T> = (usize, Option<T>);
type TerminalDecorator<T> = dyn Fn(&parser::tokenizer::Token) -> T;
type Reducer<T> = dyn Fn(&[ParseItem<T>]) -> T;

pub struct LL<T> {
    pub grammar: Grammar,
    parse_table: Table<usize>,
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
pub struct Conflict {
    pub rule: usize,
    pub symbol: usize,
    pub rhs1: usize,
    pub rhs2: usize
}

#[allow(dead_code)]
impl<T> LL<T> {
    pub fn new(grammar: Grammar) -> Result<LL<T>, Conflict> {
        let parse_table = compute_parse_table(&grammar)?;
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
                    let next_rhs = self.rhs(rule, stream.next_token().value);

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

    fn rhs(&self, rule: usize, symbol: usize) -> usize {
        if symbol == parser::tokenizer::ERROR_TOKEN_VALUE {
            return usize::MAX;
        } else {
            return *self.parse_table.at(rule, symbol);
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

fn compute_parse_table(grammar: &Grammar) -> Result<Table<usize>, Conflict> {
    let mut parse_table = Table::new(grammar.rules.len(), grammar.terminals.len(), usize::MAX);
    let sets = parser::grammar::Sets::new(&grammar);

    for (i, rule) in grammar.rules.iter().enumerate() {
        for (j, rhs) in rule.rhs.iter().enumerate() {
            let symbol = rhs[0];
            match symbol {
                parser::grammar::Symbol::Terminal(symbol_index) => {
                    add_parse_table_entry(&mut parse_table, i, symbol_index, j)?;
                },
                parser::grammar::Symbol::Nonterminal(symbol_index) => {
                    add_parse_table_entries(&mut parse_table, i, &sets.first_sets[symbol_index], j)?;

                    if sets.nullable_nonterminals.contains(&symbol_index) {
                        add_parse_table_entries(&mut parse_table, i, &sets.follow_sets[symbol_index], j)?;
                    }
                },
                parser::grammar::Symbol::Epsilon => {
                    add_parse_table_entries(&mut parse_table, i, &sets.follow_sets[i], j)?;
                }
            }
        }
    }

    return Ok(parse_table);
}

fn add_parse_table_entries(parse_table: &mut Table<usize>, rule: usize, symbols: &HashSet<usize>, rhs: usize) -> Result<(), Conflict> {
    for symbol in symbols {
        add_parse_table_entry(parse_table, rule, *symbol, rhs)?;
    }

    return Ok(());
}

fn add_parse_table_entry(parse_table: &mut Table<usize>, rule: usize, symbol: usize, rhs: usize) -> Result<(), Conflict> {
    if *parse_table.at(rule, symbol) == usize::MAX {
        *parse_table.at_mut(rule, symbol) = rhs;
        return Ok(());
    } else {
        let conflict = Conflict { rule, symbol, rhs1: *parse_table.at(rule, symbol), rhs2: rhs };
        return Err(conflict);
    }
}