#include "symbol.h"
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper function to check if a character is a valid identifier character in
// assembly
static bool
is_valid_assembly_char (char c)
{
  return isalnum (c) || c == '_'; // Allows alphanumeric and underscore
}

static char *
mutate_func_name (const char *cFunctionName)
{
  if (!cFunctionName)
    {
      return NULL;
    }

  size_t len = strlen (cFunctionName);
  char *assemblyFunctionName = (char *)malloc (len + 1);

  if (!assemblyFunctionName)
    {
      return NULL;
    }

  int i, j = 0;
  for (i = 0; i < len; i++)
    {
      if (is_valid_assembly_char (cFunctionName[i]))
        {
          assemblyFunctionName[j++] = cFunctionName[i];
        }
      else
        {
          // Replace invalid characters.  Common strategies:
          //  -  Use underscores, as in this example
          //  -  Use hex encoding of the character
          //  -  Drop the character if it's not essential to the function's
          //  purpose (but this could lead to name collisions)

          assemblyFunctionName[j++] = '_';
        }
    }

  assemblyFunctionName[j] = '\0';

  return assemblyFunctionName;
}

struct SymbolInfo *
symbol_make_local_var (const char *name, int stack_offset,
                       struct Expr *definition_node)
{
  struct SymbolInfo *info = malloc (sizeof (struct SymbolInfo));
  if (!info)
    {
      perror ("malloc");
      exit (EXIT_FAILURE);
    }
  info->name = strdup (name);
  if (!info->name)
    {
      perror ("strdup");
      free (info);
      exit (EXIT_FAILURE);
    }
  info->kind = SYM_LOCAL_VAR;
  info->location.stack_offset = stack_offset;
  info->definition_node = definition_node;
  return info;
}

struct SymbolInfo *
symbol_make_global_var (const char *name, const char *global_asm_label,
                        struct Expr *definition_node)
{
  struct SymbolInfo *info = malloc (sizeof (struct SymbolInfo));
  if (!info)
    {
      perror ("malloc");
      exit (EXIT_FAILURE);
    }
  info->name = strdup (name);
  if (!info->name)
    {
      perror ("strdup");
      free (info);
      exit (EXIT_FAILURE);
    }
  info->kind = SYM_GLOBAL_VAR;
  info->location.global_asm_label = mutate_func_name (global_asm_label);
  if (!info->location.global_asm_label)
    {
      perror ("strdup");
      free (info->name);
      free (info);
      exit (EXIT_FAILURE);
    }
  info->definition_node = definition_node;
  return info;
}

struct SymbolInfo *
symbol_make_builtin_func (const char *name, struct LispValue *builtin_val,
                          struct Expr *definition_node)
{
  struct SymbolInfo *info = malloc (sizeof (struct SymbolInfo));
  if (!info)
    {
      perror ("malloc");
      exit (EXIT_FAILURE);
    }
  info->name = strdup (name);
  if (!info->name)
    {
      perror ("strdup");
      free (info);
      exit (EXIT_FAILURE);
    }
  info->kind = SYM_BUILTIN_FUNC;
  info->location.builtin_val = builtin_val;
  info->definition_node = definition_node;
  return info;
}

struct SymbolInfo *
symbol_make_user_func (const char *name, const char *global_asm_label,
                       struct Expr *definition_node)
{
  struct SymbolInfo *info = malloc (sizeof (struct SymbolInfo));
  if (!info)
    {
      perror ("malloc");
      exit (EXIT_FAILURE);
    }
  info->name = strdup (name);
  if (!info->name)
    {
      perror ("strdup");
      free (info);
      exit (EXIT_FAILURE);
    }
  info->kind = SYM_USER_FUNC;
  info->location.global_asm_label = mutate_func_name (global_asm_label);
  if (!info->location.global_asm_label)
    {
      perror ("strdup");
      free (info->name);
      free (info);
      exit (EXIT_FAILURE);
    }
  info->definition_node = definition_node;
  return info;
}

struct SymbolInfo *
symbol_make_special_form (const char *name, struct Expr *definition_node)
{
  struct SymbolInfo *info = malloc (sizeof (struct SymbolInfo));
  if (!info)
    {
      perror ("malloc");
      exit (EXIT_FAILURE);
    }
  info->name = strdup (name);
  if (!info->name)
    {
      perror ("strdup");
      free (info);
      exit (EXIT_FAILURE);
    }
  info->kind = SYM_SPECIAL_FORM;
  info->definition_node = definition_node;
  return info;
}

void
symbol_info_free (struct SymbolInfo *info)
{
  if (!info)
    return;
  free (info->name);
  if (info->kind == SYM_GLOBAL_VAR || info->kind == SYM_USER_FUNC)
    {
      free (info->location.global_asm_label);
    }
  // No need to free builtin_val, as it points to static runtime data.
  // No need to free definition_node, as the AST owns that memory.
  free (info);
}

//  hash function for strings (DJB2)
static size_t
hash_string (const char *str, size_t capacity)
{
  unsigned long hash = 5381;
  int c;
  while ((c = *str++))
    {
      hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
  return hash % capacity;
}

static size_t
find_next_prime (size_t n)
{
  if (n <= 2)
    return 2;
  if (n % 2 == 0)
    n++;

  while (1)
    {
      size_t i;
      for (i = 3; i * i <= n; i += 2)
        {
          if (n % i == 0)
            {
              n += 2;
              break;
            }
        }
      if (i * i > n)
        return n;
    }
}

static void _symbol_map_resize (struct SymbolMap *map, size_t new_capacity);

struct SymbolMap *
symbol_map_create (void)
{
  const unsigned int INITIAL_CAPACITY = 17;
  struct SymbolMap *map = malloc (sizeof (struct SymbolMap));
  if (!map)
    {
      perror ("malloc");
      exit (EXIT_FAILURE);
    }

  map->capacity = find_next_prime (INITIAL_CAPACITY);
  map->size = 0;
  map->buckets = calloc (
      map->capacity,
      sizeof (
          struct SymbolMapElement)); // calloc initializes to zero (NULL keys)
  if (!map->buckets)
    {
      perror ("calloc");
      free (map);
      exit (EXIT_FAILURE);
    }

  return map;
}

void
symbol_map_emplace (struct SymbolMap *map, const char *key,
                    struct SymbolInfo *val)
{
  const unsigned int LOAD_FACTOR_THRESHOLD = 0.7;
  if ((double)map->size / map->capacity > LOAD_FACTOR_THRESHOLD)
    {
      _symbol_map_resize (map, find_next_prime (map->capacity + 1));
    }

  size_t index = hash_string (key, map->capacity);
  size_t original_index = index;

  while (map->buckets[index].key != NULL)
    {
      if (strcmp (map->buckets[index].key, key) == 0)
        {
          symbol_info_free (map->buckets[index].val);
          map->buckets[index].val = val;
          return;
        }
      index = (index + 1) % map->capacity;
      if (index == original_index)
        {
          fprintf (stderr,
                   "Error: Symbol map is full, cannot emplace key '%s'.\n",
                   key);
          exit (EXIT_FAILURE);
        }
    }

  map->buckets[index].key = strdup (key);
  if (!map->buckets[index].key)
    {
      perror ("strdup");
      exit (EXIT_FAILURE);
    }
  map->buckets[index].val = val;
  map->size++;
}

struct SymbolInfo *
symbol_map_lookup (struct SymbolMap *map, const char *key)
{
  size_t index = hash_string (key, map->capacity);
  size_t original_index = index;

  while (map->buckets[index].key != NULL)
    {
      if (strcmp (map->buckets[index].key, key) == 0)
        {
          return map->buckets[index].val;
        }
      index = (index + 1) % map->capacity;
      if (index == original_index)
        {
          break;
        }
    }
  return NULL;
}

void
symbol_map_free (struct SymbolMap *map)
{
  if (!map)
    return;

  for (size_t i = 0; i < map->capacity; i++)
    {
      if (map->buckets[i].key != NULL)
        {
          free (map->buckets[i].key);
          symbol_info_free (map->buckets[i].val);
        }
    }
  free (map->buckets);
  free (map);
}

static void
_symbol_map_resize (struct SymbolMap *map, size_t new_capacity)
{
  struct SymbolMapElement *old_buckets = map->buckets;
  size_t old_capacity = map->capacity;

  map->capacity = new_capacity;
  map->buckets = calloc (map->capacity, sizeof (struct SymbolMapElement));
  if (!map->buckets)
    {
      perror ("calloc");
      exit (EXIT_FAILURE);
    }
  map->size = 0;

  for (size_t i = 0; i < old_capacity; i++)
    {
      if (old_buckets[i].key != NULL)
        {
          symbol_map_emplace (map, old_buckets[i].key, old_buckets[i].val);
          free (old_buckets[i].key);
        }
    }
  free (old_buckets);
}