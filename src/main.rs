mod regex;

#[inline(never)]
pub fn f(node : &regex::parser::Node) {
}

fn main() {
    if let Ok(node) = regex::Parser::parse("[a-x]*bc") {
        let nodes = vec![node];
        let encoding = regex::Encoding::new(&nodes);
        let nfa = regex::NFA::new(&nodes, &encoding);
    }
}