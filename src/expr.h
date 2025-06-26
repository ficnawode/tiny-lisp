#ifndef EXPR_H
#define EXPR_H
#include <stdlib.h>

#include "token.h"

struct Expr;
struct Atom;
struct ExprVector;

struct Atom
{
  enum AtomType
  {
    ATOM_TYPE_SYMBOL,
    ATOM_TYPE_NUMBER,
    ATOM_TYPE_STRING
  } type;

  union AtomValue
  {
    char *symbol;
    double number;
    char *string;
  } value;
};

struct Atom atom_symbol_make (char *s);
struct Atom atom_number_make (double d);
struct Atom atom_string_make (char *s);

void atom_cleanup (struct Atom *a);

struct ExprVector
{
  struct Expr *elements;
  size_t len;
  size_t capacity;
};

struct Expr
{
  enum ExprType
  {
    S_TYPE_ATOM,
    S_TYPE_LIST,
    S_TYPE_ERROR
  } type;

  union ExprValue
  {
    struct Atom atom_val;
    struct ExprVector list_val;
    const char *error_msg;
  } val;

  size_t start_line;
  size_t start_col;
  size_t end_line;
  size_t end_col;
};

struct Expr expr_atom_make (struct Token *token);
struct Expr expr_list_make (struct ExprVector content, size_t start_line,
                            size_t start_col, size_t end_line, size_t end_col);
void expr_cleanup (struct Expr *e);

struct ExprVector exprvector_create (void);
void exprvector_append (struct ExprVector *list, struct Expr expr);
void exprvector_cleanup (struct ExprVector *list);

void print_atom (struct Atom *atom);
void print_exprvector (const struct ExprVector *list, int depth);
void print_expr (const struct Expr *expr, int depth);
void pretty_print_ast (const struct ExprVector *program);

#endif