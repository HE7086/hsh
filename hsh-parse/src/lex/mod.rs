mod token;

use std::iter::Peekable;
use std::ops::Deref;
use token::*;

use std::str::CharIndices;
use itertools::{Itertools, MultiPeek};

#[derive(Debug)]
struct Token<'a> {
    source: &'a str,
    start: usize,
    length: usize,
}

#[derive(Debug)]
struct LexerContext {
    // previous character is backslash
    escape: bool,
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
            escape: false,
            quotes: Vec::new(),
            operator: false,
            comment: false,
        }
    }

    fn reset(&mut self) {
        self.escape = false;
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
        while self.chars.next_if(|&(_, c)| c.is_whitespace()).is_some() {}
        while let Some(&(_, c)) = self.chars.peek() {
            // If the end of input is recognized, the current token (if any) shall be delimited.
            if ctx.operator && !ctx.quoted() && form_valid_operator(self.get_str(&result), c) {
                // If the previous character was used as part of an operator and the current character
                // is not quoted and can be used with the previous characters to form an operator,
                // it shall be used as part of that (operator) token.
                result.push(self.chars.next().unwrap());
                continue;
            }
            if ctx.operator && !form_valid_operator(self.get_str(&result), c) {
                // If the previous character was used as part of an operator and
                // the current character cannot be used with the previous characters to form an operator,
                // the operator containing the previous character shall be delimited.
                break;
            }
            if "\\'\"".contains(c) && !ctx.quoted() {
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
                result.push(self.chars.next().unwrap());
                continue;
                //todo
            }
            if "\\'\"".contains(c) && ctx.quoted() && ctx.quote() == c {
                ctx.quotes.pop();
                result.push(self.chars.next().unwrap());
                continue;
            }
            if "$`".contains(c) && !ctx.quoted() {
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
                continue;
            }
            if is_operator_begin(c) && !ctx.quoted() {
                // If the current character is not quoted and can be used as the first character of a new operator,
                // the current token (if any) shall be delimited.
                // The current character shall be used as the beginning of the next (operator) token.
                if result.is_empty() {
                    ctx.operator = true;
                    result.push(self.chars.next().unwrap());
                    continue;
                } else {
                    break;
                }
            }
            if c.is_whitespace() && !ctx.quoted() {
                // If the current character is an unquoted <blank>,
                // any token containing the previous character is delimited
                // and the current character shall be discarded.
                self.chars.next();
                break;
            }
            if !result.is_empty() {
                // If the previous character was part of a word,
                // the current character shall be appended to that word.
                result.push(self.chars.next().unwrap());
                if ctx.quoted() && ctx.quote() == '\\' {
                    ctx.quotes.pop();
                }
                continue;
            }
            if '#' == c {
                // If the current character is a '#', it and all subsequent characters up to,
                // but excluding, the next <newline> shall be discarded as a comment.
                // The <newline> that ends the line is not considered part of the comment.
                ctx.comment = true;
                while self.chars.next_if(|&(_, c)| c != '\n').is_some() {}
                continue;
            }
            {
                // The current character is used as the start of a new word.
                result.push(self.chars.next().unwrap());
                continue;
            }
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
        test_group("a b c", &[
            ("a", 0, 1),
            ("b", 2, 1),
            ("c", 4, 1),
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

    // #[test]
    // fn test2() {
    //     test_group("echo \"hello, world!\" | sed 's/hello/hi/g' | grep -o world >> output.txt 2>&1", &[
    //         ("echo", 0, 4),
    //         ("\"hello, world!\"", 5, 15),
    //         ("|", 21, 1),
    //         ("sed", 23, 3),
    //         ("'s/hello/hi/g'", 27, 14),
    //         ("|", 42, 1),
    //         ("grep", 44, 4),
    //         ("-o", 49, 2),
    //         ("world", 52, 5),
    //         (">>", 0, 1),
    //         ("output.txt", 0, 1),
    //         ("2>&1", 0, 1),
    //     ]);
    // }

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
