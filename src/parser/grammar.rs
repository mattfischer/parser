use std::collections::HashSet;

#[derive(Copy, Clone)]
pub enum Symbol {
    Terminal(usize),
    Nonterminal(usize),
    Epsilon
}

pub type RHS = Vec<Symbol>;

pub struct Rule {
    pub lhs: String,
    pub rhs: Vec<RHS>
}

pub struct Grammar {
    pub terminals: Vec<String>,
    pub rules: Vec<Rule>,
    pub start_rule: usize
}

impl Grammar {
    pub fn new(terminals: Vec<String>, rules: Vec<Rule>, start_rule: usize) -> Grammar {
        Grammar { terminals, rules, start_rule }
    }

    pub fn terminal_index(&self, name: &str) -> Option<usize> {
        return self.terminals.iter().position(|x| x == name);
    }

    pub fn rule_index(&self, name: &str) -> Option<usize> {
        return self.rules.iter().position(|x| x.lhs == name);
    }

    #[allow(dead_code)]
    pub fn print(&self) {
        for rule in &self.rules {
            print!("<{}>: ", rule.lhs);
            for (i, rhs) in rule.rhs.iter().enumerate() {
                for symbol in rhs {
                    match symbol {
                        Symbol::Terminal(index) => print!("{}", self.terminals[*index]),
                        Symbol::Nonterminal(index) => print!("<{}>", self.rules[*index].lhs),
                        Symbol::Epsilon => print!("0")
                    }
                    print!(" ");
                }

                if i != rule.rhs.len() - 1 {
                    print!("| ");
                }
            }
            println!();
        }
    }
}

#[derive(Clone, Copy)]
enum SetIndex {
    FirstSet(usize),
    FollowSet(usize),
    NullableNonterminals
}

pub struct Sets {
    pub first_sets: Vec<HashSet<usize>>,
    pub follow_sets: Vec<HashSet<usize>>,
    pub nullable_nonterminals: HashSet<usize>
}

impl Sets {
    pub fn new(grammar: &Grammar) -> Sets {
        let first_sets = vec![HashSet::new(); grammar.rules.len()];
        let follow_sets = vec![HashSet::new(); grammar.rules.len()];
        let nullable_nonterminals = HashSet::new();

        let mut sets = Sets { first_sets, follow_sets, nullable_nonterminals };
        sets.populate(grammar);

        return sets;
    }

    #[allow(dead_code)]
    pub fn print(&self, grammar: &Grammar) {
        println!("First sets:");
        for (i, rule) in grammar.rules.iter().enumerate() {
            print!("<{}>: ", rule.lhs);
            for index in &self.first_sets[i] {
                print!("{} ", grammar.terminals[*index]);
            }
            println!();
        }
        println!();

        println!("Follow sets:");
        for (i, rule) in grammar.rules.iter().enumerate() {
            print!("<{}>: ", rule.lhs);
            for index in &self.follow_sets[i] {
                print!("{} ", grammar.terminals[*index]);
            }
            println!();
        }
        println!();

        println!("Nullable nonterminals:");
        for index in &self.nullable_nonterminals {
            print!("<{}> ", grammar.rules[*index].lhs);
        }
        println!();
    }

    fn populate(&mut self, grammar: &Grammar) {
        let mut changed = true;
        while changed {
            changed = false;
            for (i, rule) in grammar.rules.iter().enumerate() {
                for rhs in &rule.rhs {
                    let (changed_1, is_nullable) = self.add_first_set(SetIndex::FirstSet(i), rhs);
                    changed |= changed_1;
                    if is_nullable {
                        let changed_2 = self.add_symbol(SetIndex::NullableNonterminals, i);
                        changed |= changed_2;
                    }

                    for (j, symbol) in rhs.iter().enumerate() {
                        if let Symbol::Nonterminal(symbol_index) = symbol {
                            let (changed_3, nullable) = self.add_first_set(SetIndex::FollowSet(*symbol_index), &rhs[j+1..]);
                            changed |= changed_3;
                            if nullable {
                                let changed_4 = self.add_set(SetIndex::FollowSet(*symbol_index), SetIndex::FollowSet(i));
                                changed |= changed_4;
                            }
                        }
                    }
                }
            }
        }
    }

    fn add_first_set(&mut self, set_index: SetIndex, symbols: &[Symbol]) -> (bool, bool) {
        let mut is_nullable = true;
        let mut changed = false;
        for symbol in symbols {
            let changed_1 = match symbol {
                Symbol::Terminal(symbol_index) => self.add_symbol(set_index, *symbol_index),
                Symbol::Nonterminal(symbol_index) => self.add_set(set_index, SetIndex::FirstSet(*symbol_index)),
                _ => false
            };
            changed |= changed_1;

            if !self.is_nullable(symbol) {
                is_nullable = false;
                break;
            }
        }

        return (changed, is_nullable);
    }

    fn get_set_mut(&mut self, set_index: SetIndex) -> &mut HashSet<usize> {
        let set =
        match set_index {
            SetIndex::FirstSet(index) => &mut self.first_sets[index],
            SetIndex::FollowSet(index) => &mut self.follow_sets[index],
            SetIndex::NullableNonterminals => &mut self.nullable_nonterminals 
        };

        return set;
    }

    fn get_set(&self, set_index: SetIndex) -> &HashSet<usize> {
        let set =
        match set_index {
            SetIndex::FirstSet(index) => &self.first_sets[index],
            SetIndex::FollowSet(index) => &self.follow_sets[index],
            SetIndex::NullableNonterminals => &self.nullable_nonterminals 
        };

        return set;
    }

    fn add_symbol(&mut self, set_index: SetIndex, symbol: usize) -> bool {
        let set = self.get_set_mut(set_index);

        return set.insert(symbol);
    }

    fn add_set(&mut self, target_index: SetIndex, source_index: SetIndex) -> bool {
        let set = self.get_set(source_index).clone();
        let target_set = self.get_set_mut(target_index);

        let mut changed = false;
        for s in set.iter() {
            changed = changed || target_set.insert(*s);
        }

        return changed;
    }

    fn is_nullable(&self, symbol: &Symbol) -> bool {
        match symbol {
            Symbol::Terminal(_) => return false,
            Symbol::Epsilon => return true,
            Symbol::Nonterminal(index) => return self.nullable_nonterminals.contains(index)
        }
    }
}