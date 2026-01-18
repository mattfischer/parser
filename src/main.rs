mod parser;
mod regex;
mod util;

use parser::Tokenizer;

use std::fs::File;
use std::io::BufReader;

fn main() {
    let configurations = vec![
        vec![parser::tokenizer::Pattern::new("foo", "x*", 0)]
    ];

    let tokenizer = Tokenizer::new(configurations, 1, 2);
    
    if let Ok(file) = File::open("") {
        let reader = BufReader::new(file);
        let mut stream = parser::tokenizer::Stream::new(tokenizer, Box::new(reader));

        stream.next_token();
    }
}