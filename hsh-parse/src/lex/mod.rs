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
    chars: CharIndices<'a>,
}

impl<'a> Lexer<'a> {
    fn new(source: &'a str) -> Self {
        Self {
            source,
            chars: source.char_indices(),
        }
    }

    fn next(&mut self) -> Option<Token<'a>> {
        let result: Vec<(usize, char)> = self
            .chars
            .by_ref()
            .skip_while(|&(_, c)| c.is_whitespace())
            .take_while(|&(_, c)| !c.is_whitespace())
            .collect();
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
    fn test1() {
        let mut lex = Lexer::new("a bc d efg");

        let results = [
            ("a", 0, 1),
            ("bc", 2, 2),
            ("d", 5, 1),
            ("efg", 7, 3),
        ];

        for (char, start, length) in results {
            let token = lex.next();
            assert_eq!(token.as_ref().is_some(), true);
            assert_eq!(token.as_ref().unwrap().source, char);
            assert_eq!(token.as_ref().unwrap().start, start);
            assert_eq!(token.as_ref().unwrap().length, length);
        }
        let token = lex.next();
        assert_eq!(token.is_none(), true);
    }

    #[test]
    fn test2() {
        let mut lex = Lexer::new("a    bc\n  d \t  efg \n\n");

        let results = [
            ("a", 0, 1),
            ("bc", 5, 2),
            ("d", 10, 1),
            ("efg", 15, 3),
        ];

        for (char, start, length) in results {
            let token = lex.next();
            assert_eq!(token.is_some(), true);
            assert_eq!(token.as_ref().unwrap().source, char);
            assert_eq!(token.as_ref().unwrap().start, start);
            assert_eq!(token.as_ref().unwrap().length, length);
        }
        assert_eq!(lex.next().is_none(), true);
    }

    #[test]
    fn test_one() {
        let mut lex = Lexer::new("abcdefg");
        let token = lex.next();
        assert_eq!(token.is_some(), true);
        assert_eq!(token.as_ref().unwrap().source, "abcdefg");
        assert_eq!(token.as_ref().unwrap().start, 0);
        assert_eq!(token.as_ref().unwrap().length, 7);
    }

    #[test]
    fn test_empty() {
        let mut lex = Lexer::new("");
        assert_eq!(lex.next().is_none(), true);

        let mut lex = Lexer::new("  \n   ");
        assert_eq!(lex.next().is_none(), true);
    }

    #[test]
    fn test_mixed() {
        let mut lex = Lexer::new("\t \na\t \n");
        let token = lex.next();
        assert_eq!(token.is_some(), true);
        assert_eq!(token.as_ref().unwrap().source, "a");
        assert_eq!(token.as_ref().unwrap().start, 3);
        assert_eq!(token.as_ref().unwrap().length, 1);
        assert_eq!(lex.next().is_none(), true);
    }

    #[test]
    fn test_unicode() {
        let mut lex = Lexer::new("测试 äöü ☺☺☺☺");
        let results = [
            ("测试", 0, 6),
            ("äöü", 7, 6),
            ("☺☺☺☺", 14, 12),
        ];
        for (char, start, length) in results {
            let token = lex.next();
            assert_eq!(token.is_some(), true);
            assert_eq!(token.as_ref().unwrap().source, char);
            assert_eq!(token.as_ref().unwrap().start, start);
            assert_eq!(token.as_ref().unwrap().length, length);
        }
        assert_eq!(lex.next().is_none(), true);
    }
}
