// #[cfg(test)]
// mod tests {
//     use super::*;
//
//     #[test]
//     fn run() {
//         test_group("$(())", &["$(())"]);
//     }
//
//     #[test]
//     fn test_with_location() {
//         test_group_with_location("", &[]);
//         test_group_with_location("a", &[
//             ("a", 0, 1),
//         ]);
//         test_group_with_location("a ", &[
//             ("a", 0, 1),
//         ]);
//         test_group_with_location(" a ", &[
//             ("a", 1, 1),
//         ]);
//         test_group_with_location("a b", &[
//             ("a", 0, 1),
//             ("b", 2, 1),
//         ]);
//         test_group_with_location("a  b", &[
//             ("a", 0, 1),
//             ("b", 3, 1),
//         ]);
//         test_group_with_location("a  b  ", &[
//             ("a", 0, 1),
//             ("b", 3, 1),
//         ]);
//         test_group_with_location("a b c", &[
//             ("a", 0, 1),
//             ("b", 2, 1),
//             ("c", 4, 1),
//         ]);
//         test_group_with_location("a bc def", &[
//             ("a", 0, 1),
//             ("bc", 2, 2),
//             ("def", 5, 3),
//         ]);
//         test_group_with_location(" a  bc   def", &[
//             ("a", 1, 1),
//             ("bc", 4, 2),
//             ("def", 9, 3),
//         ]);
//         test_group_with_location(" a \nbc  \ndef", &[
//             ("a", 1, 1),
//             ("bc", 4, 2),
//             ("def", 9, 3),
//         ]);
//         test_group_with_location(" 'a' 'bc def' \"123\"", &[
//             ("'a'", 1, 3),
//             ("'bc def'", 5, 8),
//             ("\"123\"", 14, 5),
//         ]);
//         test_group_with_location("测试 äöü ☺☺☺", &[
//             ("测试", 0, 6),
//             ("äöü", 7, 6),
//             ("☺☺☺", 14, 9),
//         ]);
//         test_group_with_location("<<|>>", &[
//             ("<<", 0, 2),
//             ("|", 2, 1),
//             (">>", 3, 2),
//         ]);
//         test_group_with_location("1>2", &[
//             ("1", 0, 1),
//             (">", 1, 1),
//             ("2", 2, 1),
//         ]);
//         test_group_with_location("a\\\nb\nc", &[
//             ("ab", 0, 4),
//             ("c", 5, 1),
//         ]);
//         test_group_with_location("a#b #c\nd", &[
//             ("a#b", 0, 3),
//             ("d", 7, 1),
//         ]);
//         test_group_with_location("``", &[
//             ("``", 0, 2),
//         ]);
//         test_group_with_location("()", &[
//             ("()", 0, 2),
//         ]);
//         test_group_with_location("`a b` a(a b)b", &[
//             ("`a b`", 0, 5),
//             ("a(a b)b", 6, 7),
//         ]);
//         test_group_with_location("$", &[
//             ("$", 0, 1),
//         ]);
//         test_group_with_location("${} $() $(())", &[
//             ("${}", 0, 3),
//             ("$()", 4, 3),
//             ("$(())", 8, 5),
//         ]);
//     }
//
//     #[test]
//     fn test_tokens() {
//         test_group("'('", &["'('"]);
//     }
//
//     #[test]
//     fn test_quote() {
//         test_group("'\"\"'", &["'\"\"'"]);
//         test_group("\"''\"", &["\"''\""]);
//         test_group("\"\\\"\\\"\"", &["\"\\\"\\\"\""]);
//         test_group("a\\ b c\\\nd", &["a\\ b", "cd"]);
//     }
//
//     #[test]
//     fn test_invalid_quote() {
//         let token = Lexer::new("\\").next();
//         assert!(token.is_err());
//         assert_eq!(token.unwrap_err(), LexerError::UnexpectedEOF("\\".to_string()));
//     }
//
//     #[test]
//     fn test_normal() {
//         test_group("echo \"hello, world!\" | sed 's/hello/hi/g' | grep -o world >> output.txt 2>&1", &[
//             "echo",
//             "\"hello, world!\"",
//             "|",
//             "sed",
//             "'s/hello/hi/g'",
//             "|",
//             "grep",
//             "-o",
//             "world",
//             ">>",
//             "output.txt",
//             "2",
//             ">&",
//             "1"
//         ]);
//     }
//
//     #[test]
//     fn test_empty() {
//         test_group("", &[]);
//         test_group(" ", &[]);
//         test_group("\t", &[]);
//         test_group("\n", &[]);
//         test_group(" \t \t", &[]);
//         test_group("\\\n", &[]);
//     }
//
//     fn test_group_with_location(source: &str, results: &[(&str, usize, usize)]) {
//         let mut lex = Lexer::new(source);
//
//         for &(char, start, length) in results {
//             let token = lex.next();
//             assert!(token.is_ok());
//
//             let token = token.unwrap();
//             assert_eq!(token.source.as_str(), char);
//             assert_eq!(token.start, start);
//             assert_eq!(token.length, length);
//         }
//         let token = lex.next();
//         assert!(token.is_err());
//         assert_eq!(token.unwrap_err(), LexerError::EndOfText);
//     }
//
//     fn test_group(source: &str, results: &[&str]) {
//         let mut lex = Lexer::new(source);
//
//         for &chars in results {
//             let token = lex.next();
//             assert!(token.is_ok());
//             assert_eq!(token.unwrap().source.as_str(), chars);
//         }
//         let token = lex.next();
//         assert!(token.is_err());
//         assert_eq!(token.unwrap_err(), LexerError::EndOfText);
//     }
// }
