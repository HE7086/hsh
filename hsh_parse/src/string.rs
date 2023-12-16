use nom::bytes::complete::{escaped_transform, is_not, take_while};
use nom::character::complete::{alpha0, alpha1, alphanumeric1, anychar, char, none_of};
use nom::{IResult, Parser};
use nom::branch::alt;
use nom::bytes::streaming::{is_a, tag};
use nom::combinator::{map, map_opt, value};
use nom::multi::{fold_many0, fold_many1};
use nom::sequence::{delimited, preceded};

// nom::character::complete::char <=> primitive type char
type Character = char;

fn parse_line_continuation(input: &str) -> IResult<&str, Option<Character>> {
    preceded(
        char('\\'),
        value(None, char('\n'))
    ).parse(input)
}

fn parse_escaped_char(input: &str) -> IResult<&str, Character> {
    preceded(
        char('\\'),
        alt((
            value('$', char('$')),
            value('`', char('`')),
            value('\"', char('"')),
            value('\n', char('n')),
            value('\\', char('\\')),
        )),
    ).parse(input)
}

fn parse_single_quoted_string(input: &str) -> IResult<&str, String> {
    delimited(
        char('\''),
        fold_many0(
            alt((
                parse_line_continuation,
                map(none_of("'"), Some),
            )),
            String::new,
            |mut string: String, c: Option<Character>| {
                c.map(|x| string.push(x));
                string
            },
        ),
        char('\''),
    ).parse(input)
}

fn parse_double_quoted_string(input: &str) -> IResult<&str, String> {
    delimited(
        char('\"'),
        fold_many0(
            alt((
                parse_line_continuation,
                map(parse_escaped_char, Some),
                map(none_of("\""), Some),
            )),
            String::new,
            |mut string: String, s: Option<Character>| {
                s.map(|x| string.push(x));
                string
            },
        ),
        char('\"'),
    ).parse(input)
}

fn parse_string(input: &str) -> IResult<&str, String> {
    alt((
        parse_single_quoted_string,
        parse_double_quoted_string,
    )).parse(input)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn run() {
        assert_eq!(parse_string("''"), Ok(("", String::from(""))));
        assert_eq!(parse_string("'a'"), Ok(("", String::from("a"))));
        assert_eq!(parse_string("'\n'"), Ok(("", String::from("\n"))));
        assert_eq!(parse_string("'测试'"), Ok(("", String::from("测试"))));
        assert_eq!(parse_string("'a\\\nb'"), Ok(("", String::from("ab"))));

        assert_eq!(parse_string("\"\""), Ok(("", String::from(""))));
        assert_eq!(parse_string("\"a\""), Ok(("", String::from("a"))));
        assert_eq!(parse_string("\"\n\""), Ok(("", String::from("\n"))));
        assert_eq!(parse_string("\"测试\""), Ok(("", String::from("测试"))));
        assert_eq!(parse_string("\"\\$\\`\\\"\\\\\""), Ok(("", String::from(r#"$`"\"#))));
        assert_eq!(parse_string("\"\\a\""), Ok(("", String::from("\\a"))));
        assert_eq!(parse_string("\"a\\\nb\""), Ok(("", String::from("ab"))));
    }
}