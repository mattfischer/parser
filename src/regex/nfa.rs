use crate::regex;
use regex::parser::Node;
use regex::Encoding;

type Symbol = regex::encoding::CodePoint;
type Transition = (Symbol, usize);

#[derive(Default)]
struct State {
    transitions: Vec<Transition>,
    epsilon_transitions: Vec<usize>
}

pub struct NFA {
    states: Vec<State>,
    start_state: usize,
    accept_states: Vec<usize>
}

impl NFA {
    pub fn new(nodes: &Vec<Node>, encoding: &Encoding) -> NFA {
        let mut nfa = NFA { states: Vec::new(), start_state: 0, accept_states: Vec::new() };
        nfa.start_state = nfa.add_state();

        for node in nodes {
            let start = nfa.add_state();
            let accept = nfa.add_state();

            nfa.populate(node, &encoding, start, accept);

            nfa.add_epsilon_transition(nfa.start_state, start);
            nfa.accept_states.push(accept);
        }

        return nfa;
    }

    pub fn add_state(&mut self) -> usize {
        let result = self.states.len();
        self.states.push(State::default());

        return result;
    }

    fn populate(&mut self, node: &Node, encoding: &Encoding, start_state: usize, accept_state: usize) {
        match node {
            Node::Symbol(symbol) => {
                self.add_transition(start_state, encoding.code_point(*symbol), accept_state)
            },
            Node::CharacterClass(ranges) => {
                for range in ranges {
                    for symbol in encoding.code_point_ranges(*range) {
                        self.add_transition(start_state, symbol, accept_state)
                    }
                }
            },
            Node::Sequence(nodes) => {
                let mut current = start_state;
                for node in nodes {
                    let next = self.add_state();
                    self.populate(node, encoding, current, next);
                    current = next;
                }
                self.add_epsilon_transition(current, accept_state);
            },
            Node::ZeroOrOne(node) => {
                let first = self.add_state();
                self.add_epsilon_transition(start_state, first);

                let next = self.add_state();
                self.add_epsilon_transition(next, accept_state);

                self.populate(node, encoding, first, next);
                self.add_epsilon_transition(first, next);
            },
            Node::ZeroOrMore(node) => {
                let first = self.add_state();
                self.add_epsilon_transition(start_state, first);

                let next = self.add_state();
                self.add_epsilon_transition(next, accept_state);

                self.populate(node, encoding, first, next);
                self.add_epsilon_transition(first, next);
                self.add_epsilon_transition(next, first);
            },
            Node::OneOrMore(node) => {
                let first = self.add_state();
                self.add_epsilon_transition(start_state, first);

                let next = self.add_state();
                self.add_epsilon_transition(next, accept_state);

                self.populate(node, encoding, first, next);
                self.add_epsilon_transition(next, first);
            },
            Node::OneOf(nodes) => {
                let new_start = self.add_state();
                self.add_epsilon_transition(start_state, new_start);

                let new_accept = self.add_state();
                self.add_epsilon_transition(new_accept, accept_state);

                for node in nodes {
                    self.populate(node, encoding, new_start, new_accept);
                }
            }
                
        }
    }

    fn add_transition(&mut self, from_state: usize, symbol: Symbol, to_state: usize) {
        self.states[from_state].transitions.push((symbol, to_state));
    }

    fn add_epsilon_transition(&mut self, from_state: usize, to_state: usize) {
        self.states[from_state].epsilon_transitions.push(to_state);
    }
}