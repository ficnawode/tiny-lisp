#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

struct S_Expression;

enum S_ExprType
{
  EXPR_ATOM_SYMBOL,
  EXPR_ATOM_NUMBER,
  EXPR_ATOM_STRING,
  EXPR_IF,
  EXPR_DEFINE,
  EXPR_LAMBDA,
  EXPR_CALL,
  EXPR_QUOTE_FORM
};

struct Atom
{
  char *val_str;
  double val_num;
};

struct IfExpr
{
  struct S_Expression *condition;
  struct S_Expression *then_branch;
  struct S_Expression *else_branch;
};

struct DefineExpr
{
  struct S_Expression *name;
  struct S_Expression *value;
};

struct LambdaExpr
{
  struct S_Expression **params;
  int param_count;
  int param_capacity;
  struct S_Expression **body;
  int body_count;
  int body_capacity;
};

struct CallExpr
{
  struct S_Expression *operator_expr;
  struct S_Expression **arguments;
  int arg_count;
  int arg_capacity;
};

struct QuoteForm
{
  struct S_Expression *quoted_expr;
};

struct S_Expression
{
  enum S_ExprType type;
  int start_line;
  int start_col;
  int end_line;
  int end_col;

  union
  {
    struct Atom atom;
    struct IfExpr if_expr;
    struct DefineExpr define_expr;
    struct LambdaExpr lambda_expr;
    struct CallExpr call_expr;
    struct QuoteForm quore_form;
  } data;
};

struct ParserContext
{
  struct LexerContext lexer;
  struct Token current_token;
};

void parser_advance (struct ParserContext *ctx);
void parser_error (struct ParserContext *ctx, const char *message);
void parser_cleanup (struct ParserContext *ctx);
void parser_create (struct ParserContext *ctx);
void parser_expect_token (struct ParserContext *ctx);

#endif