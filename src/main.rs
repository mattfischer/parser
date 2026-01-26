mod parser;
mod regex;
mod util;

use parser::DefReader;
use parser::algorithm::LL;
use parser::algorithm::LR;

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
    if let Some((tokenizer, mut parser)) = make_parser() {
        parser.add_terminal_decorator("NUMBER", |token| token.text.parse::<f32>().unwrap_or_default());

        parser.add_reducer("root", |items| items[0].1.unwrap());

        let minus = parser.grammar.terminal_index("-").unwrap();
        parser.add_reducer("E", move |items| {                
            let mut it = items.iter();
            let (_, first_value) = it.next().unwrap();
            let mut result = first_value.unwrap();
            while let Some((index, _)) = it.next() {
                if let Some((_, Some(value))) = it.next() {
                    result = if *index == minus { result - value } else { result + value };
                }
            }
            return result;
        });

        let divide = parser.grammar.terminal_index("/").unwrap();
        parser.add_reducer("T", move |items| {                
            let mut it = items.iter();
            let (_, first_value) = it.next().unwrap();
            let mut result = first_value.unwrap();
            while let Some((index, _)) = it.next() {
                if let Some((_, Some(value))) = it.next() {
                    result = if *index == divide { result / value } else { result * value };
                }
            }
            return result;
        });

        let lparen = parser.grammar.terminal_index("(").unwrap();
        parser.add_reducer("F", move |items| {
            let idx = if items[0].0 == lparen { 1 } else { 0 };
            return items[idx].1.unwrap_or_default();
        });

        let text = "2 * (2 + 3)";
        let reader = BufReader::new(text.as_bytes());
        let stream = parser::tokenizer::Stream::new(tokenizer, Box::new(reader));

        if let Some(result) = parser.parse(stream) {
            println!("Result: {result}");
        }
    }
}

fn make_parser<ParseData>() -> Option<(parser::Tokenizer, parser::algorithm::LR<ParseData>)> {
    let reader = BufReader::new(GRAMMAR.as_bytes());
    match DefReader::parse(Box::new(reader)) {
        Ok((tokenizer, extended_grammar)) => {            
            let grammar = extended_grammar.to_grammar();
            match LR::new_slr(grammar) {
                Ok(lr) => {
                    return Some((tokenizer, lr));
                },
                Err(conflict) => {
                    //println!("Conflict: rule {} symbol {} rhs {}/{}", conflict.rule, conflict.symbol, conflict.rhs1, conflict.rhs2);
                    return None;
                }
            }
        },
        Err(err) => {
            println!("Parse error, line {}: {}", err.line, err.message);
            return None;
        }
    }
}