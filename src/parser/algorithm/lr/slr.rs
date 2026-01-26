use crate::parser;
use parser::Grammar;
use parser::grammar::Sets;

use parser::algorithm::lr::Lookahead;
use parser::algorithm::lr::State;

use std::collections::HashSet;

pub struct LookaheadSLR {
    sets: Sets
}

impl Lookahead for LookaheadSLR {
    fn new(grammar: &Grammar, _states: &Vec<State>) -> LookaheadSLR {
        let sets = Sets::new(grammar);

        return LookaheadSLR { sets };
    }

    fn get_reduce_lookahead(&self, _state: usize, rule: usize) -> &HashSet<usize> {
        return &self.sets.follow_sets[rule];
    }
}
