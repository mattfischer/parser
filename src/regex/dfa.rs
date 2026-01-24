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
    pub start_state: usize,
    pub reject_state: usize,
    transitions: Table<usize>,
    accept_states: Vec<usize>
}

impl DFA {
    pub fn new(nfa: &NFA, encoding: &Encoding) -> DFA {
        let mut nfa_states = HashSet::new();
        nfa_states.insert(nfa.start_state);

        let mut state_sets = Vec::new();
        let mut states = Vec::new();
        let mut start_state = Self::find_or_add_state(&mut state_sets, nfa, &nfa_states);

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

        (states, start_state, accept_states) = Self::minimize(&states, start_state, &accept_states);

        states.push(State::default());
        let num_states = states.len();
        let num_code_points = encoding.num_code_points();
        let reject_state = states.len() - 1;
        let mut transitions = Table::new(num_states, num_code_points, 0);

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

    pub fn transition(&self, state: usize, symbol: Symbol) -> usize {
        return *self.transitions.at(state, symbol);
    }

    pub fn accept(&self, state: usize) -> Option<usize> {
        let accept_state = self.accept_states[state];
        if accept_state == usize::MAX {
            return None;
        } else {
            return Some(accept_state);
        }
    }

    #[allow(dead_code)]
    pub fn print(&self) {
        println!("Start state: {}", self.start_state);
        println!("Accept states:");
        for (i, accept) in self.accept_states.iter().enumerate() {
            if *accept != usize::MAX {
                println!("  {i}");
            }
        }
        println!();

        for i in 0..self.num_states {
            if i == self.reject_state {
                continue;
            }

            println!("State {i}:");
            for j in 0..self.num_code_points {
                let next_state = self.transition(i, j);
                if next_state != self.reject_state {
                    println!("  {j} -> {next_state}");
                }
            }
        }
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

    fn minimize(states: &[State], start_state: usize, accept_states: &[usize]) -> (Vec<State>, usize, Vec<usize>) {
        let mut alphabet = HashSet::new();
        for state in states.iter() {
            for transition in &state.transitions {
                alphabet.insert(*transition.0);
            }
        }

        let mut accept_sets: HashMap<usize, HashSet<usize>> = HashMap::new();
        for (i, accept_state) in accept_states.iter().enumerate() {
            if !accept_sets.contains_key(accept_state) {
                accept_sets.insert(*accept_state, HashSet::new());
            }
            accept_sets.get_mut(accept_state).unwrap().insert(i);
        }

        let mut partition = Vec::new();
        for set in accept_sets.into_values() {
            partition.push(set);
        }
        
        let mut queue = VecDeque::new();
        for i in 0..partition.len() {
            queue.push_back(i);
        }

        while queue.len() > 0 {
            let s = queue.pop_front().unwrap();
            let distinguisher = partition[s].clone();
                    
            for c in &alphabet {
                let mut inbound = HashSet::new();
                for (i, state) in states.iter().enumerate() {
                    if state.transitions.contains_key(c) && distinguisher.contains(&state.transitions[c]) {
                        inbound.insert(i);
                    }
                }

                if inbound.is_empty() {
                    continue;
                }

                for i in 0..partition.len() {
                    let mut in_set = HashSet::new();
                    let mut out_set = HashSet::new();

                    for s in &partition[i] {
                        if inbound.contains(s) {
                            in_set.insert(*s);
                        } else {
                            out_set.insert(*s);
                        }
                    }

                    if in_set.len() > 0 && out_set.len() > 0 {
                        partition[i] = in_set.clone();
                        partition.push(out_set.clone());
                        let o = partition.len() - 1;

                        if queue.contains(&i) {
                            queue.push_back(o);
                        } else {
                            if in_set.len() > out_set.len() {
                                queue.push_back(o);
                            } else {
                                queue.push_back(i);
                            }
                        }
                    }
                }
            }
        }

        let mut state_map = HashMap::new();
        for (i, part) in partition.iter().enumerate() {
            for j in part {
                state_map.insert(*j, i);
            }
        }

        let mut new_states = Vec::new();
        for part in &partition {
            let mut new_state = State::default();
            let s = part.iter().next().unwrap();
            for transition in &states[*s].transitions {
                new_state.transitions.insert(*transition.0, state_map[transition.1]);
            }
            new_states.push(new_state);
        }

        let new_start_state = state_map[&start_state];
        let mut new_accept_states = vec![usize::MAX; new_states.len()];
        for i in 0..states.len() {
            new_accept_states[state_map[&i]] = accept_states[i];
        }

        return (new_states, new_start_state, new_accept_states);
    }
}