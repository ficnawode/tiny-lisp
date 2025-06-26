#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "token.h"

struct Token
make_token (enum TokenType type, char *lexeme_buffer, int s_line, int s_col,
            int e_line, int e_col)
{
  struct Token token;
  token.type = type;
  token.lexeme = strdup (lexeme_buffer);
  if (token.lexeme == NULL)
    {
      fprintf (stdout, "Lexer Error: Failed to duplicate lexeme string\n");
      exit (EXIT_FAILURE);
    }
  token.start_line = s_line;
  token.start_col = s_col;
  token.end_line = e_line;
  token.end_col = e_col;
  return token;
}

struct Token
make_error_token (const char *message, int s_line, int s_col, int e_line,
                  int e_col)
{
  struct Token token;
  token.type = TOKEN_ERROR;

  char full_message[256];
  snprintf (full_message, sizeof (full_message), "Error at %d:%d - %s", s_line,
            s_col, message);
  token.lexeme = strdup (full_message);
  if (token.lexeme == NULL)
    {
      fprintf (stdout,
               "Lexer Error: Failed to duplicate error message string\n");
      exit (EXIT_FAILURE);
    }
  token.start_line = s_line;
  token.start_col = s_col;
  token.end_line = e_line;
  token.end_col = e_col;
  return token;
}

void
token_cleanup (struct Token *token)
{
  if (token->lexeme != NULL)
    free (token->lexeme);
}

const char *
token_type_to_string (enum TokenType type)
{
  switch (type)
    {
    case TOKEN_LPAREN:
      return "TOKEN_LPAREN";
    case TOKEN_RPAREN:
      return "TOKEN_RPAREN";
    case TOKEN_QUOTE:
      return "TOKEN_QUOTE";
    case TOKEN_SYMBOL:
      return "TOKEN_SYMBOL";
    case TOKEN_NUMBER:
      return "TOKEN_NUMBER";
    case TOKEN_STRING:
      return "TOKEN_STRING";
    case TOKEN_WHITESPACE:
      return "TOKEN_WHITESPACE";
    case TOKEN_COMMENT:
      return "TOKEN_COMMENT";
    case TOKEN_EOF:
      return "TOKEN_EOF";
    case TOKEN_ERROR:
      return "TOKEN_ERROR";
    default:
      return "UNKNOWN_TOKEN";
    }
}

void
print_token (struct Token *token)
{
  if (token->type == TOKEN_WHITESPACE)
    return;
  printf ("Type: %-15s Lexeme: \"%s\" (Pos: %d:%d to %d:%d)\n",
          token_type_to_string (token->type), token->lexeme, token->start_line,
          token->start_col, token->end_line, token->end_col);
}