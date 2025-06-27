
#include "scope.h"
#include <stdlib.h>
#include <string.h>

static struct Scope *
scope_create (struct Scope *parent)
{
  struct Scope *scope = malloc (sizeof (struct Scope));
  if (!scope)
    return NULL;

  scope->symbol_map = symbol_map_create ();
  if (!scope->symbol_map)
    {
      free (scope);
      return NULL;
    }

  scope->parent = parent;
  scope->current_stack_offset
      = (parent) ? parent->current_stack_offset : 0;

  return scope;
}

static void
scope_destroy (struct Scope *scope)
{
  if (!scope)
    return;
  symbol_map_free (scope->symbol_map);
  free (scope);
}


struct SymbolTable *
symbol_table_create (void)
{
  struct SymbolTable *st = malloc (sizeof (struct SymbolTable));
  if (!st)
    return NULL;

  st->global_scope = scope_create (NULL);
  if (!st->global_scope)
    {
      free (st);
      return NULL;
    }
  st->current_scope = st->global_scope;
  return st;
}

void
symbol_table_destroy (struct SymbolTable *st)
{
  if (!st)
    return;

  struct Scope *current = st->current_scope;
  while (current != NULL)
    {
      struct Scope *parent = current->parent;
      scope_destroy (current);
      current = parent;
    }
  free (st);
}

void
symbol_table_enter_scope (struct SymbolTable *st)
{
  struct Scope *new_scope = scope_create (st->current_scope);
  st->current_scope = new_scope;
}

void
symbol_table_exit_scope (struct SymbolTable *st)
{
  if (st->current_scope == st->global_scope)
    {
      return;
    }

  struct Scope *parent = st->current_scope->parent;
  scope_destroy (st->current_scope);
  st->current_scope = parent;
}

int
symbol_table_define (struct SymbolTable *st, struct SymbolInfo *symbol)
{
  if (!st || !st->current_scope || !symbol)
    {
      return 0;
    }

  if (symbol->kind == SYM_LOCAL_VAR)
    {
      st->current_scope->current_stack_offset += 8;
      symbol->location.stack_offset = st->current_scope->current_stack_offset;
    }

  symbol_map_emplace (st->current_scope->symbol_map, symbol->name, symbol);
  return symbol->location.stack_offset;
}

struct SymbolInfo *
symbol_table_lookup (struct SymbolTable *st, const char *name)
{
  for (struct Scope *s = st->current_scope; s != NULL; s = s->parent)
    {
      struct SymbolInfo *info = symbol_map_lookup (s->symbol_map, name);
      if (info)
        {
          return info;
        }
    }
  return NULL; 
}