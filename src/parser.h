#ifndef PARSER_H
#define PARSER_H

#include "expr.h"
#include "lexer.h"

struct ParserContext
{
  struct LexerContext lexer;
  struct Token current_token;
};

struct ParserContext parser_init (const char *source_code);
void parser_error (struct ParserContext *ctx, const char *message);
void parser_advance (struct ParserContext *ctx);
void parser_cleanup (struct ParserContext *ctx);

struct ExprList *parse_program (struct ParserContext *ctx);

struct Expr *parse_expr (struct ParserContext *ctx);
struct Expr *parse_atom (struct ParserContext *ctx);
struct Expr *parse_list (struct ParserContext *ctx);
struct Expr *parse_quoted_expression (struct ParserContext *ctx);

#endif