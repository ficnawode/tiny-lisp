#include <stdio.h>
#include <stdlib.h>

#include "parser.h"

struct ParserContext parser_make(const char *source_code) {

  struct LexerContext lexer = lexer_init(source_code);
  struct Token starting_token = make_token(TOKEN_WHITESPACE, "", 0, 0, 0, 0);

  struct ParserContext ctx = {.lexer = lexer, .current_token = starting_token};
  parser_advance(&ctx);
  return ctx;
}

void parser_error(struct ParserContext *ctx, const char *message) {
  printf("Parsing Error at %d:%d - %s (Current Token: '%s', Type: %s)\n",
         ctx->current_token.start_line, ctx->current_token.start_col, message,
         ctx->current_token.lexeme,
         token_type_to_string(ctx->current_token.type));
  exit(EXIT_FAILURE);
}

void parser_advance(struct ParserContext *ctx) {
  token_cleanup(&ctx->current_token);
  ctx->current_token = lexer_next(&ctx->lexer);
}

void parser_cleanup(struct ParserContext *ctx) { lexer_cleanup(&ctx->lexer); }

static void skip_whitespace_and_comments(struct ParserContext *ctx) {
  while (ctx->current_token.type == TOKEN_WHITESPACE ||
         ctx->current_token.type == TOKEN_COMMENT) {
    parser_advance(ctx);
  }
}

struct ExprVector parse_program(struct ParserContext *ctx) {
  struct ExprVector ast = exprvector_create();
  skip_whitespace_and_comments(ctx);
  while (ctx->current_token.type != TOKEN_EOF) {
    struct Expr e = parse_expr(ctx);
    exprvector_append(&ast, e);
    skip_whitespace_and_comments(ctx);
  }
  return ast;
}

struct Expr parse_expr(struct ParserContext *ctx) {
  switch (ctx->current_token.type) {
  case TOKEN_LPAREN:
    return parse_list(ctx);
  case TOKEN_RPAREN:
    parser_error(ctx, "Closing an unopened list");
    break;
  case TOKEN_QUOTE:
    parser_advance(ctx);
    return parse_quoted_expression(ctx);
  case TOKEN_SYMBOL:
  case TOKEN_NUMBER:
  case TOKEN_STRING:
    return parse_atom(ctx);
  case TOKEN_EOF:
    parser_error(ctx, "Unexpexted end of file (unterminated list?)");
    break;
  case TOKEN_ERROR:
    parser_error(ctx, "Token error");
    break;
  default:
    parser_error(ctx, "Illegal token");
    break;
  }
  int start_line = ctx->current_token.start_line;
  int start_col = ctx->current_token.start_col;
  int end_line = ctx->current_token.end_line;
  int end_col = ctx->current_token.end_col;
  const char *error_msg = "Unhandled token in parse_expr";

  return (struct Expr){.type = S_TYPE_ERROR,
                       .val.error_msg = error_msg,
                       .start_line = start_line,
                       .start_col = start_col,
                       .end_line = end_line,
                       .end_col = end_col};
}

struct Expr parse_atom(struct ParserContext *ctx) {
  struct Expr atom_expr = expr_atom_make(&ctx->current_token);
  if (atom_expr.type == S_TYPE_ERROR)
    parser_error(ctx, atom_expr.val.error_msg);
  parser_advance(ctx);
  return atom_expr;
}

struct Expr parse_list(struct ParserContext *ctx) {

  size_t start_line = ctx->current_token.start_line;
  size_t start_col = ctx->current_token.start_col;
  struct ExprVector list = exprvector_create();
  parser_advance(ctx);
  while (1) {
    skip_whitespace_and_comments(ctx);

    if (ctx->current_token.type == TOKEN_RPAREN) {
      break;
    }

    if (ctx->current_token.type == TOKEN_EOF) {
      parser_error(ctx, "Unterminated list, found EOF");
    }

    struct Expr e = parse_expr(ctx);
    exprvector_append(&list, e);
  }
  size_t end_line = ctx->current_token.end_line;
  size_t end_col = ctx->current_token.end_col;
  parser_advance(ctx);
  return expr_list_make(list, start_line, start_col, end_line, end_col);
}

struct Expr parse_quoted_expression(struct ParserContext *ctx) {
  size_t start_line = ctx->current_token.start_line;
  size_t start_col = ctx->current_token.start_col;
  struct ExprVector list = exprvector_create();

  struct Token t = {.type = TOKEN_SYMBOL,
                    .lexeme = (char *)"quote",
                    .start_line = start_line,
                    .start_col = start_col,
                    .end_line = start_line,
                    .end_col = start_col};
  struct Expr quote = expr_atom_make(&t);
  exprvector_append(&list, quote);
  skip_whitespace_and_comments(ctx);

  struct Expr e = parse_expr(ctx);
  exprvector_append(&list, e);

  size_t end_line = ctx->current_token.end_line;
  size_t end_col = ctx->current_token.end_col;
  return expr_list_make(list, start_line, start_col, end_line, end_col);
}
