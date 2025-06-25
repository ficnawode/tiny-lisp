#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"

static void
buffer_init (struct LexemeBuffer *buf)
{
  const size_t LEXEME_BUFFER_INITIAL_CAPACITY = 256;
  if (buf->str == NULL)
    {
      buf->str = (char *)malloc (LEXEME_BUFFER_INITIAL_CAPACITY);
      if (buf->str == NULL)
        {
          fprintf (stderr, "Lexer Error: Failed to allocate lexeme str\n");
          exit (EXIT_FAILURE);
        }
      buf->capacity = LEXEME_BUFFER_INITIAL_CAPACITY;
    }
  buf->len = 0;
  buf->str[0] = '\0';
}

static void
buffer_free (struct LexemeBuffer *buf)
{
  free (buf->str);
  buf->str = NULL;
  buf->capacity = 0;
  buf->len = 0;
}

static void
buffer_append (struct LexemeBuffer *buf, char c)
{
  if (buf->len + 1 >= buf->capacity)
    {
      int new_capacity = buf->capacity * 2;
      char *new_buffer = (char *)realloc (buf->str, new_capacity);
      if (new_buffer == NULL)
        {
          fprintf (stderr, "Lexer Error: Failed to reallocate lexeme str\n");
          exit (EXIT_FAILURE);
        }
      buf->str = new_buffer;
      buf->capacity = new_capacity;
    }
  buf->str[buf->len++] = c;
  buf->str[buf->len] = '\0';
}

static void
buffer_clear (struct LexemeBuffer *buf)
{
  buf->len = 0;
  if (buf->str != NULL)
    {
      buf->str[0] = '\0';
    }
}

static char
lexer_current_ch (struct LexerContext *ctx)
{
  if (ctx->source == NULL || ctx->source[ctx->current_pos] == '\0')
    {
      return '\0';
    }
  return ctx->source[ctx->current_pos];
}

static void
lexer_advance (struct LexerContext *ctx)
{
  if (ctx->source != NULL && ctx->source[ctx->current_pos] != '\0')
    {
      char c = ctx->source[ctx->current_pos];
      if (c == '\n')
        {
          ctx->line++;
          ctx->col = 1;
        }
      else
        {
          ctx->col++;
        }
      ctx->current_pos++;
    }
}

static struct Token
lexer_handle_single_char (struct LexerContext *ctx, enum TokenType token_type)
{
  char *lexeme = ctx->lexeme_buf.str;
  int start_line = ctx->line;
  int start_col = ctx->col;
  char c = lexer_current_ch (ctx);

  buffer_append (&ctx->lexeme_buf, c);
  lexer_advance (ctx);

  int end_line = ctx->line;
  int end_col = ctx->col;
  return make_token (token_type, lexeme, start_line, start_col, end_line,
                     end_col);
}

static struct Token
lexer_handle_whitespace (struct LexerContext *ctx)
{
  char *lexeme = ctx->lexeme_buf.str;
  int start_line = ctx->line;
  int start_col = ctx->col;
  int end_line = ctx->line;
  int end_col = ctx->col;

  while (isspace (lexer_current_ch (ctx)))
    {
      buffer_append (&ctx->lexeme_buf, lexer_current_ch (ctx));
      end_line = ctx->line;
      end_col = ctx->col;
      lexer_advance (ctx);
    }
  return make_token (TOKEN_WHITESPACE, lexeme, start_line, start_col, end_line,
                     end_col);
}

static struct Token
lexer_handle_comment (struct LexerContext *ctx)
{
  char *lexeme = ctx->lexeme_buf.str;
  int start_line = ctx->line;
  int start_col = ctx->col;
  int end_line = ctx->line;
  int end_col = ctx->col;
  while (lexer_current_ch (ctx) != '\n' && lexer_current_ch (ctx) != '\0')
    {
      buffer_append (&ctx->lexeme_buf, lexer_current_ch (ctx));
      end_line = ctx->line;
      end_col = ctx->col;
      lexer_advance (ctx);
    }
  if (lexer_current_ch (ctx) == '\n')
    {
      buffer_append (&ctx->lexeme_buf, lexer_current_ch (ctx));
      end_line = ctx->line;
      end_col = ctx->col;
      lexer_advance (ctx);
    }
  return make_token (TOKEN_COMMENT, lexeme, start_line, start_col, end_line,
                     end_col);
}

static struct Token
lexer_handle_str (struct LexerContext *ctx)
{
  char *lexeme = ctx->lexeme_buf.str;
  int start_line = ctx->line;
  int start_col = ctx->col;
  int end_line = ctx->line;
  int end_col = ctx->col;
  buffer_append (&ctx->lexeme_buf, lexer_current_ch (ctx));
  end_line = ctx->line;
  end_col = ctx->col;
  lexer_advance (ctx);
  while (lexer_current_ch (ctx) != '"' && lexer_current_ch (ctx) != '\0')
    {
      buffer_append (&ctx->lexeme_buf, lexer_current_ch (ctx));
      end_line = ctx->line;
      end_col = ctx->col;
      lexer_advance (ctx);
    }
  if (lexer_current_ch (ctx) == '\0')
    {
      return make_error_token ("Unterminated string literal", start_line,
                               start_col, end_line, end_col);
    }
  buffer_append (&ctx->lexeme_buf, lexer_current_ch (ctx));
  end_line = ctx->line;
  end_col = ctx->col;
  lexer_advance (ctx);
  return make_token (TOKEN_STRING, lexeme, start_line, start_col, end_line,
                     end_col);
}

static int
is_symbol_char (char c)
{
  return isalnum (c) || strchr ("!$%&*+-./:<=>?@^_~", c) != NULL;
}

static struct Token
lexer_handle_symbol (struct LexerContext *ctx)
{
  char *lexeme = ctx->lexeme_buf.str;
  int start_line = ctx->line;
  int start_col = ctx->col;
  int end_line = ctx->line;
  int end_col = ctx->col;

  while (lexer_current_ch (ctx) != '\0' && !isspace (lexer_current_ch (ctx))
         && !strchr ("()';\"", lexer_current_ch (ctx)))
    {
      char current = lexer_current_ch (ctx);
      if (!isdigit (current) && !strchr ("+-", current)
          && !is_symbol_char (current))
        {
          break;
        }
      buffer_append (&ctx->lexeme_buf, current);
      end_line = ctx->line;
      end_col = ctx->col;
      lexer_advance (ctx);
    }

  char *endptr;
  strtod (lexeme, &endptr);

  if (*endptr == '\0'
      && (strcmp (lexeme, "+") != 0 && strcmp (lexeme, "-") != 0))
    {
      bool contains_digit = false;
      for (int i = 0; lexeme[i] != '\0'; ++i)
        {
          if (isdigit (lexeme[i]))
            {
              contains_digit = true;
              break;
            }
        }

      if (contains_digit)
        {
          return make_token (TOKEN_NUMBER, lexeme, start_line, start_col,
                             end_line, end_col);
        }
    }

  return make_token (TOKEN_SYMBOL, lexeme, start_line, start_col, end_line,
                     end_col);
}

static struct Token
lexer_handle_error (struct LexerContext *ctx)
{
  char *lexeme = ctx->lexeme_buf.str;
  int start_line = ctx->line;
  int start_col = ctx->col;
  buffer_append (&ctx->lexeme_buf, lexer_current_ch (ctx));
  int end_line = ctx->line;
  int end_col = ctx->col;
  lexer_advance (ctx);
  char error_msg[64];
  snprintf (error_msg, sizeof (error_msg), "Illegal character: '%s'", lexeme);
  return make_error_token (error_msg, start_line, start_col, end_line,
                           end_col);
}

struct LexerContext
lexer_init (const char *source_code)
{

  struct LexerContext ctx;

  ctx.source = source_code;
  ctx.current_pos = 0;
  ctx.line = 1;
  ctx.col = 1;

  ctx.lexeme_buf.str = NULL;
  ctx.lexeme_buf.len = 0;
  ctx.lexeme_buf.capacity = 0;
  buffer_init (&ctx.lexeme_buf);
  return ctx;
}

void
lexer_cleanup (struct LexerContext *ctx)
{
  buffer_free (&ctx->lexeme_buf);
  ctx->source = NULL;
  ctx->current_pos = 0;
  ctx->line = 1;
  ctx->current_pos = 1;
}

struct Token
lexer_next (struct LexerContext *ctx)
{
  buffer_clear (&ctx->lexeme_buf);
  char c = lexer_current_ch (ctx);

  if (c == '\0')
    {
      return lexer_handle_single_char (ctx, TOKEN_EOF);
    }
  if (isspace (c))
    {
      return lexer_handle_whitespace (ctx);
    }
  if (c == ';')
    {
      return lexer_handle_comment (ctx);
    }

  switch (c)
    {
    case '(':
      return lexer_handle_single_char (ctx, TOKEN_LPAREN);
    case ')':
      return lexer_handle_single_char (ctx, TOKEN_RPAREN);
    case '\'':
      return lexer_handle_single_char (ctx, TOKEN_QUOTE);
    }

  if (c == '"')
    {
      return lexer_handle_str (ctx);
    }
  if (isdigit (c) || c == '+' || c == '-' || is_symbol_char (c))
    {
      return lexer_handle_symbol (ctx);
    }

  return lexer_handle_error (ctx);
}
