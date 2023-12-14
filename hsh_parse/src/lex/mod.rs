use std::iter::Peekable;
use std::ops::Deref;
use std::str::CharIndices;

use itertools::Itertools;

use token::*;

mod token;

#[derive(Debug)]
struct Token<'a> {
    source: &'a str,
    start: usize,
    length: usize,
}

#[derive(Debug)]
struct LexerContext {
    // current quoting character
    quotes: Vec<char>,
    // previous character is part of an operator
    operator: bool,
    // current character is commented
    comment: bool,
}

impl LexerContext {
    fn new() -> Self {
        Self {
            quotes: Vec::new(),
            operator: false,
            comment: false,
        }
    }

    fn reset(&mut self) {
        self.quotes.clear();
        self.operator = false;
        self.comment = false;
    }

    fn quoted(&self) -> bool {
        !self.quotes.is_empty()
    }

    fn quote(&self) -> char {
        *self.quotes.last().unwrap()
    }
}

#[derive(Debug)]
struct Lexer<'a> {
    source: &'a str,
    chars: Peekable<CharIndices<'a>>,
}

impl<'a> Lexer<'a> {
    fn new(source: &'a str) -> Self {
        Self {
            source,
            chars: source.char_indices().peekable(),
        }
    }

    fn get_str(&self, vec: &Vec<(usize, char)>) -> &'a str {
        let start = vec.first().unwrap().0;
        let last = vec.last().unwrap();
        let end = last.0 + last.1.len_utf8();

        self.source[start..end].as_ref()
    }

    fn get_token(&self, vec: &Vec<(usize, char)>) -> Token<'a> {
        let start = vec.first().unwrap().0;
        let last = vec.last().unwrap();
        let length = last.0 + last.1.len_utf8() - start;

        Token {
            source: &self.source[start..start + length],
            start,
            length,
        }
    }
}

impl<'a> Iterator for Lexer<'a> {
    type Item = Token<'a>;

    fn next(&mut self) -> Option<Self::Item> {
        let mut ctx = LexerContext::new();
        let mut result = Vec::<(usize, char)>::new();
        while let Some(&(_, c)) = self.chars.peek() {
            // If the end of input is recognized, the current token (if any) shall be delimited.
            match c {
                _ if ctx.operator && !ctx.quoted() && form_valid_operator(self.get_str(&result), c) => {
                    // If the previous character was used as part of an operator and the current character
                    // is not quoted and can be used with the previous characters to form an operator,
                    // it shall be used as part of that (operator) token.
                }
                _ if ctx.operator && !form_valid_operator(self.get_str(&result), c) => {
                    // If the previous character was used as part of an operator and
                    // the current character cannot be used with the previous characters to form an operator,
                    // the operator containing the previous character shall be delimited.
                    break;
                }
                '\'' | '"' if !ctx.quoted() => {
                    // If the current character is <backslash>, single-quote, or double-quote and it is not quoted,
                    // it shall affect quoting for subsequent characters up to the end of the quoted text.
                    // The rules for quoting are as described in Quoting.
                    // During token recognition no substitutions shall be actually performed,
                    // and the result token shall contain exactly the characters
                    // that appear in the input (except for <newline> joining), unmodified,
                    // including any embedded or enclosing quotes or substitution operators,
                    // between the <quotation-mark> and the end of the quoted text.
                    // The token shall not be delimited by the end of the quoted field.
                    ctx.quotes.push(c);
                }
                '\'' | '"' if ctx.quoted() && ctx.quote() == c => {
                    ctx.quotes.pop();
                }
                '\\' if !ctx.quoted() => {
                    result.push(self.chars.next().unwrap());
                    if self.chars.peek().is_none() {
                        panic!("unmatched escape sequence");
                    }
                }
                '$' | '`' if !ctx.quoted() => {
                    // If the current character is an unquoted '$' or '`',
                    // the shell shall identify the start of any candidates
                    // for parameter expansion (Parameter Expansion),
                    // command substitution (Command Substitution), or arithmetic expansion (Arithmetic Expansion)
                    // from their introductory unquoted character sequences:
                    // '$' or "${", "$(" or '`', and "$((", respectively.
                    // The shell shall read sufficient input to determine
                    // the end of the unit to be expanded (as explained in the cited sections).
                    // While processing the characters,
                    // if instances of expansions or quoting are found nested within the substitution,
                    // the shell shall recursively process them in the manner specified for the construct that is found.
                    // The characters found from the beginning of the substitution to its end,
                    // allowing for any recursion necessary to recognize embedded constructs,
                    // shall be included unmodified in the result token,
                    // including any embedded or enclosing substitution operators or quotes.
                    // The token shall not be delimited by the end of the substitution.

                    // TODO
                }
                _ if is_operator_begin(c) && !ctx.quoted() => {
                    // If the current character is not quoted and can be used as the first character of a new operator,
                    // the current token (if any) shall be delimited.
                    // The current character shall be used as the beginning of the next (operator) token.
                    if !result.is_empty() {
                        break;
                    }
                    ctx.operator = true;
                }
                _ if c.is_whitespace() && !ctx.quoted() => {
                    // If the current character is an unquoted <blank>,
                    // any token containing the previous character is delimited
                    // and the current character shall be discarded.
                    self.chars.next();
                    if result.is_empty() {
                        continue;
                    } else {
                        break;
                    }
                }
                _ if !result.is_empty() => {
                    // If the previous character was part of a word,
                    // the current character shall be appended to that word.
                }
                '#' => {
                    // If the current character is a '#', it and all subsequent characters up to,
                    // but excluding, the next <newline> shall be discarded as a comment.
                    // The <newline> that ends the line is not considered part of the comment.
                    ctx.comment = true;
                    while self.chars.next_if(|&(_, c)| c != '\n').is_some() {}
                    continue;
                }
                _ => {
                    // The current character is used as the start of a new word.
                }
            }
            result.push(self.chars.next().unwrap());
        }
        if result.is_empty() {
            return None;
        }

        Some(self.get_token(&result))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn run() {
        test_group_with_location("a b c", &[
            ("a", 0, 1),
            ("b", 2, 1),
            ("c", 4, 1),
        ]);
    }

    #[test]
    fn test_with_location() {
        test_group_with_location("a", &[
            ("a", 0, 1),
        ]);
        test_group_with_location("a ", &[
            ("a", 0, 1),
        ]);
        test_group_with_location(" a ", &[
            ("a", 1, 1),
        ]);
        test_group_with_location("a b", &[
            ("a", 0, 1),
            ("b", 2, 1),
        ]);
        test_group_with_location("a  b", &[
            ("a", 0, 1),
            ("b", 3, 1),
        ]);
        test_group_with_location("a  b  ", &[
            ("a", 0, 1),
            ("b", 3, 1),
        ]);
        test_group_with_location("a b c", &[
            ("a", 0, 1),
            ("b", 2, 1),
            ("c", 4, 1),
        ]);
        test_group_with_location("a bc def", &[
            ("a", 0, 1),
            ("bc", 2, 2),
            ("def", 5, 3),
        ]);
        test_group_with_location(" a  bc   def", &[
            ("a", 1, 1),
            ("bc", 4, 2),
            ("def", 9, 3),
        ]);
        test_group_with_location(" a \nbc  \ndef", &[
            ("a", 1, 1),
            ("bc", 4, 2),
            ("def", 9, 3),
        ]);
        test_group_with_location(" 'a' 'bc def' \"123\"", &[
            ("'a'", 1, 3),
            ("'bc def'", 5, 8),
            ("\"123\"", 14, 5),
        ]);
        test_group_with_location("测试 äöü ☺☺☺", &[
            ("测试", 0, 6),
            ("äöü", 7, 6),
            ("☺☺☺", 14, 9),
        ]);
        test_group_with_location("<<|>>", &[
            ("<<", 0, 2),
            ("|", 2, 1),
            (">>", 3, 2),
        ]);
        test_group_with_location("1>2", &[
            ("1", 0, 1),
            (">", 1, 1),
            ("2", 2, 1),
        ]);
        test_group_with_location("\\\n", &[
            ("\\\n", 0, 2),
        ]);
        test_group_with_location("a\\\nb\nc", &[
            ("a\\\nb", 0, 4),
            ("c", 5, 1),
        ]);
        test_group_with_location("a#b #c\nd", &[
            ("a#b", 0, 3),
            ("d", 7, 1),
        ]);
    }

    #[test]
    fn test_quote() {
        test_group("'\"\"'", &["'\"\"'"]);
        test_group("\"''\"", &["\"''\""]);
        test_group("\"\\\"\\\"\"", &["\"\\\"\\\"\""]);
        test_group("a\\ b c\\\nd", &["a\\ b", "c\\\nd"]);
    }

    #[test]
    #[should_panic(expected = "unmatched escape sequence")]
    fn test_invalid_quote() {
       Lexer::new("\\").next();
    }

    #[test]
    fn test_normal() {
        test_group("echo \"hello, world!\" | sed 's/hello/hi/g' | grep -o world >> output.txt 2>&1", &[
            "echo",
            "\"hello, world!\"",
            "|",
            "sed",
            "'s/hello/hi/g'",
            "|",
            "grep",
            "-o",
            "world",
            ">>",
            "output.txt",
            "2",
            ">&",
            "1"
        ]);
    }

    #[test]
    fn test_empty() {
        test_group("", &[]);
        assert!(Lexer::new("").next().is_none());
        assert!(Lexer::new(" ").next().is_none());
        assert!(Lexer::new("\t").next().is_none());
        assert!(Lexer::new("\n").next().is_none());
        assert!(Lexer::new(" \t \t").next().is_none());
    }

    fn test_group_with_location(source: &str, results: &[(&str, usize, usize)]) {
        let mut lex = Lexer::new(source);

        for &(char, start, length) in results {
            let token = lex.next();
            assert!(token.is_some());

            let token = token.unwrap();
            assert_eq!(token.source, char);
            assert_eq!(token.start, start);
            assert_eq!(token.length, length);
        }
        let token = lex.next();
        assert!(token.is_none());
    }

    fn test_group(source: &str, results: &[&str]) {
        let mut lex = Lexer::new(source);

        for &chars in results {
            let token = lex.next();
            assert!(token.is_some());
            assert_eq!(token.unwrap().source, chars);
        }
        assert!(lex.next().is_none());
    }
}
