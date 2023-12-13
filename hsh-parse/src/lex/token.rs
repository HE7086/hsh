use std::mem::variant_count;

// https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html

enum Token {
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
}

const OPERATORS: [&str; 17] = [
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
    "&",
    "|",
    "(",
    ")",
    ";",
    "<",
    ">",
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


fn form_valid_operator(base: &str, char: char) -> bool {
    OPERATORS.iter().any(|operator| operator == format!("{}{}", base, char))
}

fn is_valid_operator(str: &str) -> bool {
    OPERATORS.iter().any(|operator| operator == str)

}

fn form_valid_reserved_word(base: &str, char: char) -> bool {
    RESERVED_WORD.iter().any(|word| {
        word == format!("{}{}", base, char)
    })
}

fn is_valid_reserved_word(str: &str) -> bool {
    RESERVED_WORD.iter().any(|operator| operator == str)
}
