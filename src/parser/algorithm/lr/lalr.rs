use crate::parser;
use parser::Grammar;
use parser::grammar::Sets;
use parser::grammar::Symbol;

use parser::algorithm::lr::Lookahead;
use parser::algorithm::lr::parse_table::State;

use std::collections::{HashMap, HashSet};

pub struct LookaheadLALR {
    follow_per_state_sets: HashMap<(usize, usize), HashSet<usize>>
}

impl Lookahead for LookaheadLALR {
    fn new(grammar: &Grammar, states: &Vec<State>) -> LookaheadLALR {
        let follow_per_state_sets = compute_follow_per_state_sets(grammar, states);

        return LookaheadLALR { follow_per_state_sets };
    }

    fn get_reduce_lookahead(&self, state: usize, rule: usize) -> &HashSet<usize> {
        return &self.follow_per_state_sets[&(state, rule)];
    }
}

fn compute_follow_per_state_sets(grammar: &Grammar, states: &Vec<State>) -> HashMap<(usize, usize), HashSet<usize>> {
    let mut new_nonterminals = Vec::new();

    let find_nonterminal = |new_nonterminals: &Vec<(usize, usize)>, state: usize, rule: usize| -> usize {
        if let Some(pos) = new_nonterminals.iter().position(|x| *x == (state, rule)) {
            return pos;
        } else {
            return usize::MAX;
        }
    };

    let mut new_rules = Vec::new();
    for (i, state) in states.iter().enumerate() {
        for item in &state.items {
            if item.pos == 0 && find_nonterminal(&new_nonterminals, i, item.rule) == usize::MAX {
                new_nonterminals.push((i, item.rule));

                let name = format!("{}@{}", grammar.rules[item.rule].lhs, i);
                new_rules.push(parser::grammar::Rule { lhs: name, rhs: Vec::new() });
            }
        }
    }

    let mut reduction_starts = HashMap::new();
    for (i, state) in states.iter().enumerate() {
        for item in &state.items {
            if item.pos == 0 {
                let rhs = &grammar.rules[item.rule].rhs[item.rhs];

                let mut new_rhs = parser::grammar::RHS::new();
                let mut state_num = i;
                for symbol in rhs {
                    match symbol {
                        Symbol::Nonterminal(index) => {
                            let s = find_nonterminal(&new_nonterminals, state_num, *index);
                            new_rhs.push(Symbol::Nonterminal(s));
                            state_num = states[state_num].transitions[&(index + grammar.terminals.len())];
                        },
                        Symbol::Terminal(index) => {
                            new_rhs.push(*symbol);
                            state_num = states[state_num].transitions[index];
                        },
                        Symbol::Epsilon => {
                            new_rhs.push(*symbol);
                        }
                    }
                }

                let r = find_nonterminal(&new_nonterminals, i, item.rule);
                new_rules[r].rhs.push(new_rhs);

                if !reduction_starts.contains_key(&(state_num, item.rule)) {
                    reduction_starts.insert((state_num, item.rule), HashSet::new());
                }
                reduction_starts.get_mut(&(state_num, item.rule)).unwrap().insert(i);
            }
        }
    }

    let new_grammar = Grammar::new(grammar.terminals.clone(), new_rules, grammar.start_rule);
    let new_sets = Sets::new(&new_grammar);

    let mut follow_per_state_sets = HashMap::new();
    for ((reduce_state, rule), states) in &reduction_starts {
        for start_state in states {
            let r = find_nonterminal(&new_nonterminals, *start_state, *rule);
            if !follow_per_state_sets.contains_key(&(*reduce_state, *rule)) {
                follow_per_state_sets.insert((*reduce_state, *rule), HashSet::new());
            }
            follow_per_state_sets.get_mut(&(*reduce_state, *rule)).unwrap().extend(new_sets.follow_sets[r].clone());
        }
    }

    return follow_per_state_sets;
}
