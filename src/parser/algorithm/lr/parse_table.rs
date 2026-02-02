use crate::parser;
use parser::Grammar;
use parser::grammar::Symbol;

use crate::util;
use util::Table;

use std::collections::{HashMap, HashSet, VecDeque};

#[derive(PartialEq, Eq, Hash, Clone, Copy)]
pub struct Item {
    pub rule: usize,
    pub rhs: usize,
    pub pos: usize
}

pub struct State {
    pub items: HashSet<Item>,
    pub transitions: HashMap<usize, usize>
}

#[derive(Copy, Clone)]
pub enum TableEntry {
    Shift(usize),
    Reduce((usize, usize)),
    Error
}

#[allow(dead_code)]
pub enum Conflict {
    ReduceReduce(usize, usize, usize),
    ShiftReduce(usize, usize)
}

pub trait Lookahead {
    fn new(grammar: &Grammar, states: &Vec<State>) -> Self;
    fn get_reduce_lookahead(&self, state: usize, rule: usize) -> &HashSet<usize>;    
}

pub struct ParseTable {
    table: Table<TableEntry>,
    accept_states: HashSet<usize>,
    num_terminals: usize
}

impl ParseTable {
    pub fn new<L: Lookahead>(grammar: &Grammar) -> Result<ParseTable, Conflict> {
        let (table, accept_states) = compute_parse_table::<L>(grammar)?;

        let parse_table = ParseTable { table, accept_states, num_terminals: grammar.terminals.len() };
        return Ok(parse_table);
    }

    pub fn is_accept(&self, state: usize) -> bool {
        return self.accept_states.contains(&state);
    }

    pub fn action_for_terminal(&self, state: usize, terminal: usize) -> &TableEntry {
        return self.table.at(state, terminal);
    }            

    pub fn action_for_rule(&self, state: usize, rule: usize) -> &TableEntry {
        return self.table.at(state, rule + self.num_terminals);
    }            
}

fn compute_parse_table<L: Lookahead>(grammar: &Grammar) -> Result<(Table<TableEntry>, HashSet<usize>), Conflict> {
    let states = compute_states(grammar);
    let lookahead = L::new(grammar, &states);

    print_states(&states, grammar, &lookahead);

    let mut table = Table::new(states.len(), grammar.rules.len() + grammar.terminals.len(), TableEntry::Error);
    let mut accept_states = HashSet::new();

    for (i, state) in states.iter().enumerate() {
        for item in &state.items {
            let rhs = &grammar.rules[item.rule].rhs[item.rhs];
            if item.pos == rhs.len() {
                for terminal in lookahead.get_reduce_lookahead(i, item.rule) {
                    match table.at(i, *terminal) {
                        TableEntry::Reduce((rule, _rhs)) => {
                            let conflict = Conflict::ReduceReduce(*terminal, *rule, item.rule);
                            return Err(conflict);
                        },
                        _ => ()
                    }

                    *table.at_mut(i, *terminal) = TableEntry::Reduce((item.rule, item.rhs));
                }

                if item.rule == grammar.start_rule {
                    accept_states.insert(i);
                }
            }
        }

        for transition in &state.transitions {
            match table.at(i, *transition.0) {
                TableEntry::Reduce((rule, _rhs)) => {
                    let conflict = Conflict::ShiftReduce(*transition.0, *rule);
                    return Err(conflict);
                },
                _ => ()
            }
            *table.at_mut(i, *transition.0) = TableEntry::Shift(*transition.1);
        }
    }
    return Ok((table, accept_states));
}

fn compute_states(grammar: &Grammar) -> Vec<State> {
    let mut states = Vec::new();

    let mut start = State { items: HashSet::new(), transitions: HashMap::new() };
    for i in 0..grammar.rules[grammar.start_rule].rhs.len() {
        start.items.insert(Item { rule: grammar.start_rule, rhs: i, pos: 0 } );
    }
    compute_closure(&mut start.items, grammar);
    states.push(start);

    let mut queue = VecDeque::new();
    queue.push_back(0);
    while let Some(index) = queue.pop_front() {
        for i in 0..(grammar.rules.len() + grammar.terminals.len()) {
            let mut new_items = HashSet::new();

            for item in &states[index].items {
                let rhs = &grammar.rules[item.rule].rhs[item.rhs];
                if item.pos < rhs.len() && symbol_index(rhs[item.pos], grammar) == i {
                    new_items.insert(Item { rule: item.rule, rhs: item.rhs, pos: item.pos + 1});
                }
            }

            if !new_items.is_empty() {
                compute_closure(&mut new_items, grammar);

                let new_state;
                match states.iter().position(|s| s.items == new_items) {
                    Some(pos) => {
                        new_state = pos;
                    },
                    None => {
                        new_state = states.len();
                        queue.push_back(new_state);
                        states.push(State { items: new_items, transitions: HashMap::new() });
                    }
                }
                states[index].transitions.insert(i, new_state);
            }
        }
    }
    
    return states;
}

fn compute_closure(items: &mut HashSet<Item>, grammar: &Grammar) {
    let mut queue = VecDeque::new();
    queue.extend(items.iter().cloned());

    while let Some(item) = queue.pop_front() {
        let rhs = &grammar.rules[item.rule].rhs[item.rhs];
        if item.pos < rhs.len() {
            match rhs[item.pos] {
                Symbol::Nonterminal(new_rule_index) => {
                    let new_rule = &grammar.rules[new_rule_index];
                    for i in 0..new_rule.rhs.len() {
                        let new_item = Item { rule: new_rule_index, rhs: i, pos: 0 };
                        if items.insert(new_item) {
                            queue.push_back(new_item);
                        }
                    }
                },
                Symbol::Epsilon => {
                    let new_item = Item { rule: item.rule, rhs: item.rhs, pos: item.pos + 1 };
                    if items.insert(new_item) {
                        queue.push_back(new_item);
                    }
                },
                Symbol::Terminal(_) => ()
            }
        }
    }
}

fn symbol_index(symbol: Symbol, grammar: &Grammar) -> usize {
    match symbol {
        Symbol::Terminal(index) => return index,
        Symbol::Nonterminal(index) => return grammar.terminals.len() + index,
        Symbol::Epsilon => return usize::MAX
    }
}

#[allow(dead_code)]
fn print_states(states: &Vec<State>, grammar: &Grammar, lookahead: &impl Lookahead) {
    for (i, state) in states.iter().enumerate() {
        println!("State {i}:");
        for item in &state.items {
            print!("  <{}>: ", grammar.rules[item.rule].lhs);
            let rhs = &grammar.rules[item.rule].rhs[item.rhs];
            for j in 0..=rhs.len() {
                if j == item.pos {
                    print!(". ");
                }

                if j == rhs.len() {
                    break;
                }

                match rhs[j] {
                    Symbol::Terminal(index) => print!("{}", grammar.terminals[index]),
                    Symbol::Nonterminal(index) => print!("<{}>", grammar.rules[index].lhs),
                    Symbol::Epsilon => print!("0")
                }
                print!(" ");
            }

            if item.pos == rhs.len() {
                print!("[ ");
                for terminal in lookahead.get_reduce_lookahead(i, item.rule) {
                    print!("{} ", grammar.terminals[*terminal]);
                }
                print!("]");
            }
            println!();
        }
        println!();

        for transition in &state.transitions {
            print!("  ");
            if *transition.0 < grammar.terminals.len() {
                print!("{}", grammar.terminals[*transition.0]);
            } else {
                print!("<{}>", grammar.rules[*transition.0 - grammar.terminals.len()].lhs);
            }
            println!(" -> {}", transition.1);
        }
        println!();
    }
}
