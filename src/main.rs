mod parser;
mod regex;
mod util;

use parser::DefReader;

use std::io::BufReader;

const GRAMMAR: &str = r#"
NUMBER: [0-9]+
IGNORE: \s

<root> : <E>
<E>: <T> ( ( '+' | '-' ) <T> )*
<T>: <F> ( ( '*' | '/' ) <F> )*
<F>: NUMBER | '(' <E> ')'
"#;

fn main() {
    let reader = BufReader::new(GRAMMAR.as_bytes());
    if let Ok((tokenizer, extended_grammar)) = DefReader::parse(Box::new(reader)) {
        let grammar = extended_grammar.to_grammar();
        grammar.print();
        println!();

        let sets = parser::grammar::Sets::new(&grammar);
        sets.print(&grammar);
    }
}