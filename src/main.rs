mod regex;

#[inline(never)]
pub fn f(node : &regex::parser::Node) {
}

fn main() {
    if let Ok(node) = regex::Parser::parse("[a-x]*?") {
        f(&node);
    }
}