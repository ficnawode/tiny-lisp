#ifndef SCOPE_H
#define SCOPE_H

#include "symbol.h"
#include <stddef.h>

struct Scope
{
  struct SymbolMap *symbol_map;
  int current_stack_offset;
  struct Scope *parent;
};

struct SymbolTable
{
  struct Scope *current_scope;
  struct Scope *global_scope;
};

struct SymbolTable *symbol_table_create (void);

void symbol_table_destroy (struct SymbolTable *st);

void symbol_table_enter_scope (struct SymbolTable *st);

void symbol_table_exit_scope (struct SymbolTable *st);

int symbol_table_define (struct SymbolTable *st, struct SymbolInfo *symbol);

struct SymbolInfo *symbol_table_lookup (struct SymbolTable *st,
                                        const char *name);

#endif