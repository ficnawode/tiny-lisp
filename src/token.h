#ifndef TOKEN_H
#define TOKEN_H

enum TokenType
{
  TOKEN_LPAREN,     // (
  TOKEN_RPAREN,     // )
  TOKEN_QUOTE,      // '
  TOKEN_SYMBOL,     // e.g., 'foo', '+', 'define', 'list?'
  TOKEN_NUMBER,     // e.g., 123, -4.5, +0.7
  TOKEN_STRING,     // e.g., "hello world"
  TOKEN_WHITESPACE, // spaces, tabs, newlines (often skipped by parsers, but
                    // requested here)
  TOKEN_COMMENT,    // ; ... to end of line (often skipped)
  TOKEN_EOF,        // End of File
  TOKEN_ERROR       // Lexing error
};

struct Token
{
  enum TokenType type;
  char *lexeme;
  int start_line;
  int start_col;
  int end_line;
  int end_col;
};
struct Token make_token (enum TokenType type, char *lexeme_buffer, int s_line,
                         int s_col, int e_line, int e_col);

struct Token make_error_token (const char *message, int s_line, int s_col,
                               int e_line, int e_col);
void token_cleanup (struct Token *token);

const char *token_type_to_string (enum TokenType type);
void print_token (struct Token *token);

#endif
