mod token;

use std::iter::Peekable;
use std::str::CharIndices;

#[derive(Debug)]
struct Token<'a> {
    source: &'a str,
    start: usize,
    length: usize,
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

    fn next(&mut self) -> Option<Token<'a>> {
        // ---------- contexts ----------

        // last char is a backslash
        let mut escape = false;

        // currently in a quoted block (" or ')
        let mut quote: Option<char> = None;

        // last char is a symbol
        let mut symbol: Option<bool> = None;
        // ------------------------------

        let mut result = Vec::<(usize, char)>::new();
        while self.chars.next_if(|&(_, c)| c.is_whitespace()).is_some() {
            // simply remove all whitespaces
        }
        while let Some(&(_, c)) = self.chars.peek() {
            let last_escape = escape;
            let last_symbol = symbol;
            match c {
                '\\' => {
                    escape = true;
                }
                '\'' | '\"' => {
                    //todo
                    escape = false;
                    if quote.is_none() {
                        quote = Some(c);
                    } else if quote.unwrap() == c {
                        quote = None;
                    }
                }
                c_ if c_.is_alphanumeric() => {
                    escape = false;
                    if !last_escape {
                        symbol = Some(false);
                    }
                    if last_symbol.unwrap_or(false) {
                        break;
                    }
                }
                c_ if c_.is_whitespace() => {
                    escape = false;
                }
                _ => { // should be all symbols and control characters
                    escape = false;
                    if !last_escape {
                        symbol = Some(true);
                    }
                    if !last_symbol.unwrap_or(true) {
                        break;
                    }
                }
            }
            if last_escape || quote.is_some() || !c.is_whitespace() {
                result.push(self.chars.next().unwrap());
            } else {
                break;
            }
        }
        if result.is_empty() {
            return None;
        }

        let first = result.first().unwrap();
        let last = result.last().unwrap();

        let start = first.0;
        let length = last.0 + last.1.len_utf8() - start;

        Some(
            Token {
                source: &self.source[start..start + length],
                start,
                length,
            }
        )
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn run() {
        test_group("\\ a\\ b\\ c def", &[
            ("\\ a\\ b\\ c", 0, 9),
            ("def", 10, 3),
        ]);
    }

    fn test_group(source: &str, results: &[(&str, usize, usize)]) {
        let mut lex = Lexer::new(source);

        for &(char, start, length) in results {
            let token = lex.next();
            assert!(token.as_ref().is_some());
            assert_eq!(token.as_ref().unwrap().source, char);
            assert_eq!(token.as_ref().unwrap().start, start);
            assert_eq!(token.as_ref().unwrap().length, length);
        }
        let token = lex.next();
        assert!(token.is_none());
    }

    #[test]
    fn test1() {
        // basic
        test_group("a bc d efg", &[
            ("a", 0, 1),
            ("bc", 2, 2),
            ("d", 5, 1),
            ("efg", 7, 3),
        ]);
        // whitespaces
        test_group("a    bc\n  d \t  efg \n\n", &[
            ("a", 0, 1),
            ("bc", 5, 2),
            ("d", 10, 1),
            ("efg", 15, 3),
        ]);
        // unicode
        test_group("测试 äöü ☺☺☺☺", &[
            ("测试", 0, 6),
            ("äöü", 7, 6),
            ("☺☺☺☺", 14, 12),
        ]);
        // escape
        test_group("\\ a\\ b\\ c def", &[
            ("\\ a\\ b\\ c", 0, 9),
            ("def", 10, 3),
        ]);
        // quotes
        test_group("\"abc\'\" def\'", &[
            ("\"abc\'\"", 0, 6),
            ("def\'", 7, 4),
        ]);
        test_group("\"abc\\ \" \" def\"", &[
            ("\"abc\\ \"", 0, 7),
            ("\" def\"", 8, 6),
        ]);
        test_group("<a >", &[
            ("<", 0, 1),
            ("a", 1, 1),
            (">", 3, 1),
        ]);
        test_group("< <<a <a<<ab > a>", &[
            ("<", 0, 1),
            ("<<", 2, 2),
            ("a", 4, 1),
            ("<", 6, 1),
            ("a", 7, 1),
            ("<<", 8, 2),
            ("ab", 10, 2),
            (">", 13, 1),
            ("a", 15, 1),
            (">", 16, 1),
        ]);
    }

    #[test]
    fn test_one() {
        let mut lex = Lexer::new("abcdefg");
        let token = lex.next();
        assert!(token.is_some());
        assert_eq!(token.as_ref().unwrap().source, "abcdefg");
        assert_eq!(token.as_ref().unwrap().start, 0);
        assert_eq!(token.as_ref().unwrap().length, 7);
    }

    #[test]
    fn test_empty() {
        let mut lex = Lexer::new("");
        assert!(lex.next().is_none());

        let mut lex = Lexer::new("  \n   ");
        assert!(lex.next().is_none());
    }

    #[test]
    fn test_mixed() {
        let mut lex = Lexer::new("\t \na\t \n");
        let token = lex.next();
        assert!(token.is_some());
        assert_eq!(token.as_ref().unwrap().source, "a");
        assert_eq!(token.as_ref().unwrap().start, 3);
        assert_eq!(token.as_ref().unwrap().length, 1);
        assert!(lex.next().is_none());
    }
}
