use crate::util;
use util::Table;

use crate::parser;
use parser::Grammar;

use std::collections::HashSet;

#[allow(dead_code)]
pub struct Conflict {
    pub rule: usize,
    pub symbol: usize,
    pub rhs1: usize,
    pub rhs2: usize
}

pub struct ParseTable {
    table: Table<usize>
}

impl ParseTable {
    pub fn new(grammar: &Grammar) -> Result<ParseTable, Conflict> {
        let table = compute_parse_table(grammar)?;
        let parse_table = ParseTable { table };

        return Ok(parse_table);
    }

    pub fn rhs(&self, rule: usize, symbol: usize) -> usize {
        if symbol == parser::tokenizer::ERROR_TOKEN_VALUE {
            return usize::MAX;
        } else {
            return *self.table.at(rule, symbol);
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