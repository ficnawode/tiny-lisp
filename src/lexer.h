#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include <stddef.h>

struct LexerContext
{
  const char *source;
  int current_pos;
  int line;
  int col;
  struct LexemeBuffer
  {
    char *str;
    size_t len;
    size_t capacity;
  } lexeme_buf;
};

struct LexerContext lexer_init (const char *source_code);

// Parser will have to free token lexeme!
struct Token lexer_next (struct LexerContext *ctx);

void lexer_cleanup (struct LexerContext *ctx);

#endif
