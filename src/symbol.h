#ifndef SYMBOL_H
#define SYMBOL_H
#include "lispvalue.h"
#include <stddef.h>

struct SymbolInfo
{
  char *name;
  enum
  {
    SYM_LOCAL_VAR,
    SYM_GLOBAL_VAR,
    SYM_BUILTIN_FUNC,
    SYM_USER_FUNC,
    SYM_SPECIAL_FORM,
  } kind;

  union
  {
    int stack_offset;       // For SYM_KIND_LOCAL_VAR: offset from RBP
    char *global_asm_label; // For SYM_KIND_GLOBAL_VAR, SYM_KIND_USER_FUNC
    struct LispValue *builtin_val; // Points to a pre-initialized
                                   // LispValue in runtime.c
  } location;

  struct Expr *definition_node;
};

struct SymbolInfo *symbol_make_local_var (const char *name, int stack_offset,
                                          struct Expr *definition_node);
struct SymbolInfo *symbol_make_global_var (const char *name,
                                           const char *global_asm_label,
                                           struct Expr *definition_node);
struct SymbolInfo *symbol_make_builtin_func (const char *name,
                                             struct LispValue *builtin_val,
                                             struct Expr *definition_node);
struct SymbolInfo *symbol_make_user_func (const char *name,
                                          const char *global_asm_label,
                                          struct Expr *definition_node);
struct SymbolInfo *symbol_make_special_form (const char *name,
                                             struct Expr *definition_node);

struct SymbolMap
{
  struct SymbolMapElement
  {
    char *key;
    struct SymbolInfo *val;
  } *buckets;

  size_t capacity;
  size_t size;
};

struct SymbolMap *symbol_map_create (void);
void symbol_map_emplace (struct SymbolMap *map, const char *key,
                         struct SymbolInfo *val);
struct SymbolInfo *symbol_map_lookup (struct SymbolMap *map, const char *key);
void symbol_map_free (struct SymbolMap *map);

#endif