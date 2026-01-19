use crate::parser;
use parser::Grammar;

pub enum Symbol {
    Terminal(usize),
    Nonterminal(usize)
}

pub enum RHSNode {
    Symbol(Symbol),
    Sequence(Vec<RHSNode>),
    OneOf(Vec<RHSNode>),
    OneOrMore(Box<RHSNode>),
    ZeroOrMore(Box<RHSNode>),
    ZeroOrOne(Box<RHSNode>)
}

pub struct Rule {
    pub lhs: String,
    pub rhs: RHSNode
}

pub struct ExtendedGrammar {
    terminals: Vec<String>,
    rules: Vec<Rule>,
    start_rule: usize
}

impl ExtendedGrammar {
    pub fn new(terminals: Vec<String>, rules: Vec<Rule>, start_rule: usize) -> ExtendedGrammar {
        ExtendedGrammar { terminals, rules, start_rule }
    }

    pub fn print(&self) {
        for rule in &self.rules {
            print!("<{}>: ", rule.lhs);
            self.print_rhs_node(&rule.rhs);
            println!();
        }
    }

    fn print_rhs_node(&self, node: &RHSNode) {
        match node {
            RHSNode::Symbol(symbol) => {
                match symbol {
                    Symbol::Nonterminal(index) => print!("<{}>", self.rules[*index].lhs),
                    Symbol::Terminal(index) => print!("{}", self.terminals[*index])
                }
            },
            RHSNode::Sequence(nodes) => {
                for (i, child) in nodes.iter().enumerate() {
                    self.print_rhs_node(child);
                    if i != nodes.len() - 1 {
                        print!(" ");
                    }
                }
            },
            RHSNode::OneOf(nodes) => {
                print!("( ");
                for (i, node) in nodes.iter().enumerate() {
                    self.print_rhs_node(node);
                    if i != nodes.len() - 1 {
                        print!(" | ");
                    }
                }
                print!(" )");
            },
            RHSNode::ZeroOrOne(child) => {
                self.print_with_parens(child);
                print!(" ?");
            },
            RHSNode::ZeroOrMore(child) => {
                self.print_with_parens(child);
                print!(" *");
            },
            RHSNode::OneOrMore(child) => {
                self.print_with_parens(child);
                print!(" +");
            }
        }
    }

    fn print_with_parens(&self, node: &RHSNode) {
        match node {
            RHSNode::Symbol(_) => self.print_rhs_node(node),
            _ => {
                print!("( ");
                self.print_rhs_node(node);
                print!(" )");
            }
        }
    }

    pub fn to_grammar(&self) -> Grammar {
        let mut grammar_rules = Vec::new();
        for rule in &self.rules {
            grammar_rules.push(parser::grammar::Rule { lhs: rule.lhs.clone(), rhs: Vec::new() });
        }

        for i in 0..grammar_rules.len() {
            self.populate_rule(&mut grammar_rules, i, &self.rules[i].rhs);
        }

        return Grammar::new(self.terminals.clone(), grammar_rules, self.start_rule);
    }

    fn populate_rule(&self, grammar_rules: &mut Vec<parser::grammar::Rule>, index: usize, rhs_node: &RHSNode) {
        let rule_name = grammar_rules[index].lhs.clone();
        match rhs_node {
            RHSNode::OneOf(nodes) => {
                for node in nodes {
                    let grammar_rhs = self.create_rhs(node, grammar_rules, &rule_name);
                    grammar_rules[index].rhs.push(grammar_rhs);
                }
            },
            _ => {
                let grammar_rhs = self.create_rhs(rhs_node, grammar_rules, &rule_name);
                grammar_rules[index].rhs.push(grammar_rhs);
            }
        }
    }

    fn create_rhs(&self, rhs_node: &RHSNode, grammar_rules: &mut Vec<parser::grammar::Rule>, rule_name: &str) -> parser::grammar::RHS {
        let mut grammar_rhs = parser::grammar::RHS::new();
        match rhs_node {
            RHSNode::Sequence(nodes) => {
                for child in nodes {
                    let grammar_symbol = self.create_symbol(child, grammar_rules, rule_name);
                    grammar_rhs.push(grammar_symbol);
                }
            },
            _ => {
                let grammar_symbol = self.create_symbol(rhs_node, grammar_rules, rule_name);
                grammar_rhs.push(grammar_symbol);
            }
        }

        return grammar_rhs;
    }

    fn create_symbol(&self, rhs_node: &RHSNode, grammar_rules: &mut Vec<parser::grammar::Rule>, rule_name: &str) -> parser::grammar::Symbol {
        let grammar_symbol;

        match rhs_node {
            RHSNode::Symbol(symbol) => {
                grammar_symbol = match symbol {
                    Symbol::Terminal(index) => parser::grammar::Symbol::Terminal(*index),
                    Symbol::Nonterminal(index) => parser::grammar::Symbol::Nonterminal(*index)
                };
            },
            RHSNode::ZeroOrOne(child) => {
                let index = grammar_rules.len();
                grammar_symbol = parser::grammar::Symbol::Nonterminal(index);

                let grammar_rule = parser::grammar::Rule { lhs: self.create_subrule_name(rule_name, grammar_rules), rhs: Vec::new() };
                grammar_rules.push(grammar_rule);

                self.populate_rule(grammar_rules, index, child);

                let grammar_rhs = vec![parser::grammar::Symbol::Epsilon];
                grammar_rules[index].rhs.push(grammar_rhs);
            },
            RHSNode::ZeroOrMore(child) => {
                let index = grammar_rules.len();
                grammar_symbol = parser::grammar::Symbol::Nonterminal(index);

                let grammar_rule = parser::grammar::Rule { lhs: self.create_subrule_name(rule_name, grammar_rules), rhs: Vec::new() };
                grammar_rules.push(grammar_rule);
                self.populate_rule(grammar_rules, index, child);
                for rhs in &mut grammar_rules[index].rhs {
                    rhs.push(parser::grammar::Symbol::Nonterminal(index));
                }

                let grammar_rhs = vec![parser::grammar::Symbol::Epsilon];
                grammar_rules[index].rhs.push(grammar_rhs);
            },
            RHSNode::OneOrMore(child) => {
                let index = grammar_rules.len();
                grammar_symbol = parser::grammar::Symbol::Nonterminal(index);
                
                let grammar_rule = parser::grammar::Rule { lhs: self.create_subrule_name(rule_name, grammar_rules), rhs: Vec::new() };
                grammar_rules.push(grammar_rule);

                let next_rule_index = grammar_rules.len();
                let grammar_rule = parser::grammar::Rule { lhs: self.create_subrule_name(rule_name, grammar_rules), rhs: Vec::new() };
                grammar_rules.push(grammar_rule);

                self.populate_rule(grammar_rules, index, child);
                for rhs in &mut grammar_rules[index].rhs {
                    rhs.push(parser::grammar::Symbol::Nonterminal(next_rule_index));
                }

                let next_rhs = grammar_rules[index].rhs.clone();
                let next_rule = &mut grammar_rules[next_rule_index];
                next_rule.rhs = next_rhs;

                let grammar_rhs = vec![parser::grammar::Symbol::Epsilon];
                next_rule.rhs.push(grammar_rhs);
            },
            RHSNode::OneOf(_) => {
                let index = grammar_rules.len();
                grammar_symbol = parser::grammar::Symbol::Nonterminal(index);

                let grammar_rule = parser::grammar::Rule{ lhs: self.create_subrule_name(rule_name, grammar_rules), rhs: Vec::new() };
                grammar_rules.push(grammar_rule);

                self.populate_rule(grammar_rules, index, rhs_node);
            },
            RHSNode::Sequence(_) => {
                grammar_symbol = parser::grammar::Symbol::Epsilon;
            }
        }

        return grammar_symbol;
    }

    fn create_subrule_name(&self, rule_name: &str, grammar_rules: &[parser::grammar::Rule]) -> String {
        let mut n = 1;
        loop {
            let subrule_name = format!("{rule_name}.{n}");

            if let Some(_) = grammar_rules.iter().position(|x| x.lhs == subrule_name) {
                n += 1;
            } else {
                return subrule_name;
            }
        }
    }
}