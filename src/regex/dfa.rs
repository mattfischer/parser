use crate::util;
use util::Table;

use crate::regex;
use regex::Encoding;
use regex::NFA;

use std::collections::HashMap;
use std::collections::HashSet;
use std::collections::VecDeque;

type Symbol = usize;

#[derive(Default)]
struct State {
    pub transitions: HashMap<Symbol, usize>
}

#[derive(Default)]
struct StateSet {
    pub nfa_states: HashSet<usize>,
    pub transitions: HashMap<Symbol, usize>
}
pub struct DFA {
    num_code_points: usize,
    num_states: usize,
    start_state: usize,
    reject_state: usize,
    transitions: Table<usize>,
    accept_states: Vec<usize>
}

impl DFA {
    pub fn new(nfa: &NFA, encoding: &Encoding) -> DFA {
        let mut nfa_states = HashSet::new();
        nfa_states.insert(nfa.start_state);

        let mut state_sets = Vec::new();
        let mut states = Vec::new();
        let start_state = Self::find_or_add_state(&mut state_sets, nfa, &nfa_states);

        let mut accept_sets: Vec<HashSet<usize>> = vec![HashSet::<usize>::default(); nfa.accept_states.len()];
        
        for (i, state_set) in state_sets.iter().enumerate() {
            let mut state = State::default();

            for transition in &state_set.transitions {
                state.transitions.insert(*transition.0, *transition.1);
            }

            for (j, accept_state) in nfa.accept_states.iter().enumerate() {
                if state_set.nfa_states.contains(accept_state) {
                    accept_sets[j].insert(i);
                }
            }

            states.push(state);
        }

        let mut accept_states = vec![usize::MAX; states.len()];
        for (i, _) in states.iter().enumerate() {
            for (j, accept_set) in accept_sets.iter().enumerate() {
                if accept_set.contains(&i) {
                    accept_states[i] = j;
                    break;
                }
            }
        }

        // TODO: minimize

        states.push(State::default());
        let num_states = states.len();
        let num_code_points = encoding.num_code_points();
        let reject_state = states.len() - 1;
        let mut transitions = Table::new(num_states, num_code_points);

        for (i, state) in states.iter().enumerate() {
            for j in 0..num_code_points {
                if state.transitions.contains_key(&j) {
                    *transitions.at_mut(i, j) = state.transitions[&j];
                } else {
                    *transitions.at_mut(i, j) = reject_state;
                }
            }
        }

        return DFA { num_code_points, num_states, start_state, reject_state, transitions, accept_states };
    }

    fn find_or_add_state(state_sets: &mut Vec<StateSet>, nfa: &NFA, nfa_states: &HashSet<usize>) -> usize {
        let mut epsilon_closure = HashSet::new();
        let mut queue = VecDeque::new();
        for state in nfa_states {
            queue.push_back(*state);
        }

        while queue.len() > 0 {
            let state = queue.pop_front().unwrap();
            if !epsilon_closure.contains(&state) {
                epsilon_closure.insert(state);
                for transition in &nfa.states[state].epsilon_transitions {
                    queue.push_back(*transition);
                }
            }
        }

        for (idx, state_set) in state_sets.iter().enumerate() {
            if state_set.nfa_states == epsilon_closure {
                return idx;
            }
        }

        let mut transitions: HashMap<usize, HashSet<usize>> = HashMap::new();
        for state in &epsilon_closure {
            for transition in &nfa.states[*state].transitions {
                if !transitions.contains_key(&transition.0) {
                    transitions.insert(transition.0, HashSet::<usize>::new());
                }

                transitions.get_mut(&transition.0).unwrap().insert(transition.1);
            }
        }

        state_sets.push(StateSet::default());
        let idx = state_sets.len() - 1;
        state_sets[idx].nfa_states = epsilon_closure;

        for transition in &transitions {
            let s = Self::find_or_add_state(state_sets, nfa, transition.1);
            state_sets.get_mut(idx).unwrap().transitions.insert(*transition.0, s);
        }

        return idx;
    }
}