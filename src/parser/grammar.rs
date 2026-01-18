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
    terminals: Vec<String>,
    rules: Vec<Rule>,
    start_rule: usize
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