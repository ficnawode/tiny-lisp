#include "expr.h"
#include <stdio.h>
#include <string.h>

char *
unquote_str (const char *lexeme)
{
  if (lexeme == NULL || strlen (lexeme) < 2 || lexeme[0] != '"'
      || lexeme[strlen (lexeme) - 1] != '"')
    {
      return strdup ("");
    }
  char *unquoted = strdup (lexeme + 1);
  if (!unquoted)
    {
      printf ("Failed to allocate unquoted string");
      exit (EXIT_FAILURE);
    }
  unquoted[strlen (unquoted) - 1] = '\0';
  return unquoted;
}

struct Atom
atom_symbol_make (char *s)
{
  struct Atom a;
  a.type = ATOM_TYPE_SYMBOL;
  a.value.symbol = strdup (s);
  return a;
}

struct Atom
atom_number_make (double d)
{
  struct Atom a;
  a.type = ATOM_TYPE_NUMBER;
  a.value.number = d;
  return a;
}

struct Atom
atom_string_make (char *s)
{
  struct Atom a;
  a.type = ATOM_TYPE_STRING;
  a.value.string = unquote_str (s);
  return a;
}

void
atom_cleanup (struct Atom *a)
{
  switch (a->type)
    {
    case ATOM_TYPE_NUMBER:
      return;
    case ATOM_TYPE_STRING:
      free (a->value.string);
      return;
    case ATOM_TYPE_SYMBOL:
      free (a->value.symbol);
      return;
    default:
      return;
    }
}

struct Expr *
expr_atom_create (struct Token *token)
{

  struct Expr *e = (struct Expr *)malloc (sizeof (struct Expr));
  e->start_line = token->start_line;
  e->end_line = token->end_line;
  e->start_col = token->start_col;
  e->end_col = token->end_col;

  switch (token->type)
    {
    case TOKEN_NUMBER:
      e->type = S_TYPE_ATOM;
      double d = strtod (token->lexeme, NULL);
      e->val.atom_val = atom_number_make (d);
      return e;
    case TOKEN_STRING:
      e->type = S_TYPE_ATOM;
      e->val.atom_val = atom_string_make (token->lexeme);
      return e;
    case TOKEN_SYMBOL:
      e->type = S_TYPE_ATOM;
      e->val.atom_val = atom_symbol_make (token->lexeme);
      return e;
    default:
      e->type = S_TYPE_ERROR;
      e->val.error_msg = "Illegal token to atom conversion";
      return e;
    }
}

struct Expr *
expr_list_create (struct ExprList *content, size_t start_line,
                  size_t start_col, size_t end_line, size_t end_col)
{

  struct Expr *e = (struct Expr *)malloc (sizeof (struct Expr));
  e->start_line = start_line;
  e->end_line = end_line;
  e->start_col = start_col;
  e->end_col = end_col;

  e->type = S_TYPE_LIST;
  e->val.list_val = content;
  return e;
}

void
expr_cleanup (struct Expr *e)
{
  switch (e->type)
    {
    case S_TYPE_ATOM:
      atom_cleanup (&e->val.atom_val);
      return;
    case S_TYPE_LIST:
      exprlist_cleanup (e->val.list_val);
      return;
    default:
      break;
    }
  free (e);
}

struct ExprList *
exprlist_create (void)
{
  const size_t LIST_INITIAL_CAPACITY = 8;
  struct ExprList *e = (struct ExprList *)malloc (sizeof (struct ExprList));
  e->capacity = LIST_INITIAL_CAPACITY;
  e->len = 0;

  e->elements = (struct Expr **)malloc (e->capacity * sizeof (struct Expr *));
  if (e->elements == NULL)
    {
      printf ("Failed to allocate expression list\n");
      exit (EXIT_FAILURE);
    }
  return e;
}

void
exprlist_append (struct ExprList *list, struct Expr *expr)
{
  if (list->len + 1 >= list->capacity)
    {
      int new_capacity = list->capacity * 2;
      struct Expr **new_elements
          = (struct Expr **)realloc (list->elements, new_capacity);
      if (new_elements == NULL)
        {
          printf ("Failed to reallocate expression list\n");
          exit (EXIT_FAILURE);
        }
      list->elements = new_elements;
      list->capacity = new_capacity;
    }
  list->elements[list->len++] = expr;
}

void
exprlist_cleanup (struct ExprList *list)
{
  for (int i = 0; i < list->len; i++)
    expr_cleanup (list->elements[i]);
  free (list->elements);
  free (list);
}

void
print_indent (int depth)
{
  for (int i = 0; i < depth; i++)
    {
      printf (" ");
    }
}

void
print_atom (struct Atom *atom)
{
  if (atom == NULL)
    {
      printf ("NULL_ATOM");
      return;
    }

  switch (atom->type)
    {
    case ATOM_TYPE_SYMBOL:
      printf ("%s", atom->value.symbol);
      break;
    case ATOM_TYPE_NUMBER:
      printf ("%g",
              atom->value.number); // %g for general floating point format
      break;
    case ATOM_TYPE_STRING:
      printf ("\"%s\"", atom->value.string); // Print with quotes
      break;
    default:
      printf ("<UNKNOWN_ATOM_TYPE>");
      break;
    }
}

void
print_exprlist (const struct ExprList *list, int depth)
{
  if (list == NULL)
    {
      printf ("NULL_LIST");
      return;
    }

  printf ("(\n");

  for (size_t i = 0; i < list->len; ++i)
    {
      print_indent (depth + 1);
      print_expr (list->elements[i], depth + 1);
      printf ("\n");
    }

  print_indent (depth);
  printf (")");
}

void
print_expr (const struct Expr *expr, int depth)
{
  if (expr == NULL)
    {
      printf ("NULL_EXPR");
      return;
    }

  switch (expr->type)
    {
    case S_TYPE_ATOM:
      print_atom (&expr->val.atom_val);
      break;
    case S_TYPE_LIST:
      print_exprlist (expr->val.list_val, depth);
      break;
    default:
      printf ("<UNKNOWN_EXPR_TYPE>");
      break;
    }
}

void
pretty_print_ast (const struct ExprList *program)
{
  if (program == NULL)
    {
      printf ("Empty Program AST.\n");
      return;
    }

  printf ("--- AST Pretty Print ---\n");
  for (size_t i = 0; i < program->len; ++i)
    {
      print_expr (program->elements[i], 0);
      printf ("\n");
    }
  printf ("------------------------\n");
}
