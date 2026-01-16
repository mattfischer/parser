mod regex;
mod util;

fn main() {
    if let Ok(matcher) = regex::Matcher::new(vec!["(a|b|c)*d".to_string()]) {
        let (num_matched, matched_pattern) = matcher.match_string("abcd", 0);
    }
}