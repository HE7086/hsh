// https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html

#[rustfmt::skip]
pub(crate) enum Token {
    // @formatter:off
    Word,
    AssignmentWord,
    Name,
    Newline,
    IONumber,

    AndIf,     // &&
    OrIf,      // ||
    DSemi,     // ;;
    DLess,     // <<
    DGreat,    // >>
    LessAnd,   // <&
    GreatAnd,  // >&
    LessGreat, // <>
    DLessDash, // <<-

    Clobber, // >|
    And,     // &
    Or,      // |
    Lb,      // (
    Rb,      // )
    Semi,    // ;
    Less,    // <
    Great,   // >

    If,
    Then,
    Else,
    Elif,
    Fi,
    Do,
    Done,
    Case,
    Esac,
    While,
    Until,
    For,

    Lbrace, // {
    Rbrace, // }
    Bang,   // !
    In,     // in
    // @formatter:on
}

pub(crate) const OPERATORS: [&str; 17] = [
    "&&",
    "||",
    ";;",
    "<<",
    ">>",
    "<&",
    ">&",
    "<>",
    "<<-",
    ">|",
    // are these operators?
    "&",
    "|",
    "(",
    ")",
    ";",
    "<",
    ">",
];

pub(crate) const OPERATORS_BEGIN: [char; 7] = [
    '&',
    '|',
    ';',
    '<',
    '>',
    '(',
    ')',
];

const RESERVED_WORD: [&str; 20] = [
    "{",
    "}",
    "!",
    "if",
    "then",
    "else",
    "elif",
    "fi",
    "do",
    "done",
    "case",
    "esac",
    "while",
    "until",
    "for",
    "in",
    "[[",
    "]]",
    "function",
    "select",
];

pub(crate) fn is_operator_begin(char: char) -> bool {
    OPERATORS.iter().any(|&operator| operator.starts_with(char))
}

pub(crate) fn form_valid_operator(base: &str, char: char) -> bool {
    OPERATORS.iter().any(|&operator| operator.starts_with(format!("{}{}", base, char).as_str()))
}

pub(crate) fn is_valid_operator(str: &str) -> bool {
    OPERATORS.iter().any(|&operator| operator == str)
}

pub(crate) fn form_valid_reserved_word(base: &str, char: char) -> bool {
    RESERVED_WORD.iter().any(|&word| word.starts_with(format!("{}{}", base, char).as_str()))
}

pub(crate) fn is_valid_reserved_word(str: &str) -> bool {
    RESERVED_WORD.iter().any(|&operator| operator == str)
}

#[cfg(test)]
mod tests {
    use std::collections::HashSet;
    use super::*;

    #[test]
    fn test_operators() {
        let expected: HashSet<char> = OPERATORS.into_iter().map(|s| s.char_indices().next().unwrap().1).collect();
        let actual: HashSet<char> = OPERATORS_BEGIN.into_iter().collect();
        assert_eq!(actual, expected, "OPERATORS_BEGIN does not match all begins of OPERATORS");
    }
}