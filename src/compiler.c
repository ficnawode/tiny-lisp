#include "compiler.h"
#include "expr.h"
#include "global_data_sections.h"
#include "lispvalue.h"
#include "scope.h"
#include "symbol.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void compile_expr (struct CompilerContext *ctx, struct Expr *expr);
static void compile_atom (struct CompilerContext *ctx, struct Atom *atom);
static void compile_list (struct CompilerContext *ctx, struct Expr *list_expr);
static void compile_define_function (struct CompilerContext *ctx,
                                     struct ExprVector *vec);
static void compile_function_call (struct CompilerContext *ctx,
                                   struct SymbolInfo *op_info,
                                   struct ExprVector *vec);
static size_t count_local_defines (struct ExprVector *define_expr_vec);

static int
new_label_id ()
{
  static int label_counter = 0;
  return label_counter++;
}

static void
sanitize_label (char *buffer, size_t buf_size, const char *prefix,
                const char *name)
{
  size_t prefix_len = strlen (prefix);

  if (buf_size < prefix_len + strlen (name) + 1)
    {
      fprintf (stderr, "Error: Buffer too small for sanitized label.\n");
      exit (EXIT_FAILURE);
    }

  strcpy (buffer, prefix);

  for (size_t i = 0; name[i] != '\0'; ++i)
    {
      buffer[prefix_len + i]
          = isalnum ((unsigned char)name[i]) ? name[i] : '_';
    }
  buffer[prefix_len + strlen (name)] = '\0';
}

static void
populate_global_scope (struct SymbolTable *st)
{
  struct SymbolInfo *define_info = symbol_make_special_form ("define", NULL);
  symbol_map_emplace (st->global_scope->symbol_map, "define", define_info);

  struct SymbolInfo *if_info = symbol_make_special_form ("if", NULL);
  symbol_map_emplace (st->global_scope->symbol_map, "if", if_info);

  struct SymbolInfo *add_info = symbol_make_builtin_func ("+", NULL, NULL);
  symbol_map_emplace (st->global_scope->symbol_map, "+", add_info);
  struct SymbolInfo *subtr_info = symbol_make_builtin_func ("-", NULL, NULL);
  symbol_map_emplace (st->global_scope->symbol_map, "-", subtr_info);
}

static void
generate_prologue (FILE *text_section)
{
  const char *prologue = "global main\n"
                         "extern lisp_add\n"
                         "extern lisp_subtract\n"
                         "extern lisp_multiply\n"
                         "extern lisp_divide\n"
                         "extern lisp_make_number\n\n";
  fprintf (text_section, prologue);
}

static void
generate_main (struct CompilerContext *ctx, struct ExprVector *program)
{
  const char *prologue = "main:\n"
                         "  push rbp\n"
                         "  mov rbp, rsp\n\n";
  fprintf (ctx->gds->text_file, prologue);

  for (size_t i = 0; i < program->len; ++i)
    {
      compile_expr (ctx, &program->elements[i]);
    }

  const char *epilogue = "\n  ; Main function epilogue\n"
                         "  mov rax, 0      ; Return 0 for success\n"
                         "  mov rsp, rbp\n"
                         "  pop rbp\n"
                         "  ret\n";
  fprintf (ctx->gds->text_file, epilogue);
}

void
compile_program (struct ExprVector *program, const char *output_filename)
{
  struct SymbolTable *sym_table = symbol_table_create ();
  populate_global_scope (sym_table);

  struct GlobalDataSections *gds = gds_create (output_filename);
  if (!gds)
    {
      symbol_table_destroy (sym_table);
      exit (EXIT_FAILURE);
    }

  struct CompilerContext ctx = { .sym_table = sym_table, .gds = gds };

  if (!ctx.gds->text_file)
    {
      fprintf (stderr, "Fatal: Could not get .text section file.\n");
      gds_close_and_finalize (gds);
      symbol_table_destroy (sym_table);
      exit (EXIT_FAILURE);
    }

  generate_prologue (ctx.gds->text_file);
  generate_main (&ctx, program);
  gds_close_and_finalize (gds);
  symbol_table_destroy (sym_table);
}

static void
compile_expr (struct CompilerContext *ctx, struct Expr *expr)
{
  switch (expr->type)
    {
    case S_TYPE_ATOM:
      compile_atom (ctx, &expr->val.atom_val);
      break;
    case S_TYPE_LIST:
      compile_list (ctx, expr);
      break;
    case S_TYPE_ERROR:
      fprintf (stderr, "Compilation error: %s\n", expr->val.error_msg);
      exit (EXIT_FAILURE);
    }
}

static void
compile_atom (struct CompilerContext *ctx, struct Atom *atom)
{
  switch (atom->type)
    {
    case ATOM_TYPE_NUMBER:
      {
        FILE *rodata = ctx->gds->rodata_file;
        int label_id = new_label_id ();
        fprintf (rodata, "L_double_%d: dq %lf\n", label_id,
                 atom->value.number);

        fprintf (ctx->gds->text_file,
                 "\n  ; Create LispValue for number %.2f on the heap\n",
                 atom->value.number);
        fprintf (ctx->gds->text_file, "  movsd xmm0, [rel L_double_%d]\n",
                 label_id);
        fprintf (ctx->gds->text_file, "  call lisp_make_number\n");
        break;
      }
    case ATOM_TYPE_SYMBOL:
      {
        struct SymbolInfo *info
            = symbol_table_lookup (ctx->sym_table, atom->value.symbol);
        if (!info)
          {
            fprintf (stderr, "Compilation error: Undefined symbol '%s'\n",
                     atom->value.symbol);
            exit (EXIT_FAILURE);
          }

        fprintf (ctx->gds->text_file, "\n  ; Load symbol '%s'\n",
                 atom->value.symbol);
        switch (info->kind)
          {
          case SYM_LOCAL_VAR:
            fprintf (ctx->gds->text_file, "  mov rax, [rbp - %d]\n",
                     info->location.stack_offset);
            break;
          case SYM_GLOBAL_VAR:
            fprintf (ctx->gds->text_file, "  mov rax, [%s]\n",
                     info->location.global_asm_label);
            break;
          default:
            fprintf (stderr,
                     "Compilation error: Cannot use '%s' as a value.\n",
                     atom->value.symbol);
            exit (EXIT_FAILURE);
          }
        break;
      }
    case ATOM_TYPE_STRING:
      fprintf (stderr, "Strings are not implemented yet.\n");
      exit (EXIT_FAILURE);
      break;
    }
}

/**
 * @brief Scans a function definition expression to count local variable
 * definitions.
 * @param define_expr_vec The vector for the entire `(define (f ...) ...)`
 * expression.
 * @return The number of local `(define var val)` forms in the function body.
 */
static size_t
count_local_defines (struct ExprVector *define_expr_vec)
{
  size_t count = 0;
  // The body starts at index 2 of a (define (f ...) body...) expression
  for (size_t i = 2; i < define_expr_vec->len; ++i)
    {
      struct Expr *stmt = &define_expr_vec->elements[i];
      if (stmt->type == S_TYPE_LIST && stmt->val.list_val.len >= 2)
        {
          struct ExprVector *list = &stmt->val.list_val;
          struct Expr *op = &list->elements[0];
          struct Expr *name = &list->elements[1];

          if (op->type == S_TYPE_ATOM
              && op->val.atom_val.type == ATOM_TYPE_SYMBOL
              && strcmp (op->val.atom_val.value.symbol, "define") == 0)
            {
              // Ensure it's a variable define `(define x ...)` and not a
              // nested func define `(define (g) ...)`
              if (name->type == S_TYPE_ATOM)
                {
                  count++;
                }
            }
        }
    }
  return count;
}

static void
compile_define_function (struct CompilerContext *ctx, struct ExprVector *vec)
{
  struct Expr *signature = &vec->elements[1];
  struct ExprVector *sig_vec = &signature->val.list_val;
  struct Expr *func_name_expr = &sig_vec->elements[0];

  if (func_name_expr->type != S_TYPE_ATOM)
    {
      fprintf (stderr, "Error: Function name must be a symbol.\n");
      exit (EXIT_FAILURE);
    }
  const char *func_name = func_name_expr->val.atom_val.value.symbol;

  char label_buf[256];
  sanitize_label (label_buf, sizeof (label_buf), "user_func_", func_name);
  char *asm_label = strdup (label_buf);

  struct SymbolInfo *func_info
      = symbol_make_user_func (func_name, asm_label, func_name_expr);
  symbol_table_define (ctx->sym_table, func_info);

  if (ctx->gds->func_file == NULL)
    {
      printf ("\nFunction file is NULL: are you trying to create a nested "
              "function? \n");
      exit (1);
    }

  fprintf (ctx->gds->func_file, "\n; ---- Function Definition: %s ----\n",
           func_name);
  fprintf (ctx->gds->func_file, "%s:\n", asm_label);
  free (asm_label);

  fprintf (ctx->gds->func_file, "  push rbp\n");
  fprintf (ctx->gds->func_file, "  mov rbp, rsp\n");

  symbol_table_enter_scope (ctx->sym_table);

  const char *arg_registers[] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };
  size_t num_params = sig_vec->len - 1;
  if (num_params > 6)
    {
      fprintf (stderr, "Error: Functions with more than 6 parameters are not "
                       "yet supported.\n");
      exit (EXIT_FAILURE);
    }

  size_t num_locals = count_local_defines (vec);
  size_t total_stack_vars = num_params + num_locals;

  if (total_stack_vars > 0)
    {
      fprintf (ctx->gds->func_file, "  sub rsp, %zu\n", total_stack_vars * 8);
    }

  for (size_t i = 0; i < num_params; ++i)
    {
      struct Expr *param_expr = &sig_vec->elements[i + 1];
      const char *param_name = param_expr->val.atom_val.value.symbol;

      struct SymbolInfo *param_info
          = symbol_make_local_var (param_name, 0, param_expr);
      int stack_offset = symbol_table_define (ctx->sym_table, param_info);

      fprintf (ctx->gds->func_file,
               "  ; Store parameter '%s' from %s to [rbp - %d]\n", param_name,
               arg_registers[i], stack_offset);
      fprintf (ctx->gds->func_file, "  mov [rbp - %d], %s\n", stack_offset,
               arg_registers[i]);
    }

  fprintf (ctx->gds->func_file, "\n  ; Function Body for %s\n", func_name);

  FILE *temp_text_file = ctx->gds->text_file;
  ctx->gds->text_file = ctx->gds->func_file;
  ctx->gds->func_file = NULL; // Prevents compiling nested functions for now
  for (size_t i = 2; i < vec->len; ++i)
    {
      compile_expr (ctx, &vec->elements[i]);
    }
  ctx->gds->func_file = ctx->gds->text_file;
  ctx->gds->text_file = temp_text_file;

  fprintf (ctx->gds->func_file, "\n  ; Epilogue for %s\n", func_name);
  fprintf (ctx->gds->func_file, "  mov rsp, rbp\n"); // Deallocate stack frame
  fprintf (ctx->gds->func_file, "  pop rbp\n");
  fprintf (ctx->gds->func_file, "  ret\n");
  fprintf (ctx->gds->func_file, "; ---- End Function: %s ----\n", func_name);

  symbol_table_exit_scope (ctx->sym_table);
}

const char *
match_builtin_function (const char *op_name)
{
  const char *extern_func = "UNKNOWN_FUNCTION";
  if (strcmp (op_name, "+") == 0)
    extern_func = "lisp_add";
  else if (strcmp (op_name, "-"))
    extern_func = "lisp_subtract";
  else if (strcmp (op_name, "*"))
    extern_func = "lisp_multiply";
  else if (strcmp (op_name, "/"))
    extern_func = "lisp_divide";
  return extern_func;
}

static void
compile_function_call (struct CompilerContext *ctx, struct SymbolInfo *op_info,
                       struct ExprVector *vec)
{
  const char *op_name = op_info->name;
  size_t num_args = vec->len - 1;

  fprintf (ctx->gds->text_file, "\n  ; --- Function Call to '%s' ---\n",
           op_name);

  if (op_info->kind == SYM_BUILTIN_FUNC)
    {
      if (strcmp (op_name, "+") == 0 || strcmp (op_name, "-") == 0)
        {
          if (num_args != 2)
            {
              fprintf (stderr,
                       "Error: Built-in '%s' requires 2 arguments, got %zu.\n",
                       op_name, num_args);
              exit (EXIT_FAILURE);
            }
          compile_expr (ctx, &vec->elements[2]);
          fprintf (ctx->gds->text_file, "  push rax\n");
          compile_expr (ctx, &vec->elements[1]);
          fprintf (ctx->gds->text_file, "  mov rdi, rax\n");
          fprintf (ctx->gds->text_file, "  pop rsi\n");

          const char *extern_func = match_builtin_function (op_name);
          fprintf (ctx->gds->text_file, "  call %s\n", extern_func);
          return;
        }
    }

  const char *arg_registers[] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };
  if (num_args > 6)
    {
      fprintf (stderr, "Error: Calling functions with more than 6 arguments "
                       "is not supported.\n");
      exit (EXIT_FAILURE);
    }

  fprintf (ctx->gds->text_file,
           "  ; Evaluate arguments (push args to stack)\n");
  for (size_t i = 1; i <= num_args; ++i)
    {
      compile_expr (ctx, &vec->elements[i]);
      fprintf (ctx->gds->text_file, "  push rax\n");
    }

  fprintf (ctx->gds->text_file,
           "  ; Load arguments from stack into registers\n");
  for (int i = num_args - 1; i >= 0; --i)
    {
      fprintf (ctx->gds->text_file, "  pop %s\n", arg_registers[i]);
    }

  fprintf (ctx->gds->text_file, "  call %s\n",
           op_info->location.global_asm_label);
  fprintf (ctx->gds->text_file,
           "  ; --- End Call to '%s', result is in RAX ---\n", op_name);
}

static void
compile_list (struct CompilerContext *ctx, struct Expr *list_expr)
{
  struct ExprVector *vec = &list_expr->val.list_val;

  if (vec->len == 0)
    {
      fprintf (stderr, "Compiling '() is not implemented yet.\n");
      exit (EXIT_FAILURE);
    }

  struct Expr *first = &vec->elements[0];
  if (first->type != S_TYPE_ATOM
      || first->val.atom_val.type != ATOM_TYPE_SYMBOL)
    {
      fprintf (stderr, "Error: Expression starting with a non-symbol.\n");
      exit (EXIT_FAILURE);
    }

  const char *op_name = first->val.atom_val.value.symbol;
  struct SymbolInfo *op_info = symbol_table_lookup (ctx->sym_table, op_name);

  if (!op_info)
    {
      fprintf (stderr, "Error: Undefined operator '%s'\n", op_name);
      exit (EXIT_FAILURE);
    }

  if (op_info->kind == SYM_SPECIAL_FORM)
    {
      if (strcmp (op_name, "define") == 0)
        {
          if (vec->len < 3)
            {
              fprintf (stderr,
                       "Error: Invalid 'define' syntax. Too few parts.\n");
              exit (EXIT_FAILURE);
            }

          struct Expr *name_part = &vec->elements[1];

          if (name_part->type == S_TYPE_ATOM)
            { // This is (define var val)
              char *symbol_name = name_part->val.atom_val.value.symbol;

              if (ctx->sym_table->current_scope
                  == ctx->sym_table->global_scope)
                {
                  // --- Global Variable Definition ---
                  compile_expr (ctx, &vec->elements[2]);
                  fprintf (ctx->gds->text_file, "  push rax\n");

                  FILE *data = ctx->gds->data_file;
                  char label_buf[256];
                  sanitize_label (label_buf, sizeof (label_buf), "G_",
                                  symbol_name);
                  char *label_name = strdup (label_buf);
                  fprintf (data, "%s: dq 0\n", label_name);

                  struct SymbolInfo *info = symbol_make_global_var (
                      symbol_name, label_name, name_part);
                  symbol_table_define (ctx->sym_table, info);
                  free (label_name);

                  fprintf (ctx->gds->text_file, "  pop rbx\n");
                  fprintf (ctx->gds->text_file, "  mov [%s], rbx\n",
                           info->location.global_asm_label);
                  fprintf (ctx->gds->text_file, "  mov rax, rbx\n");
                }
              else // local variable
                {
                  fprintf (ctx->gds->text_file,
                           "\n  ; Local define for '%s'\n", symbol_name);
                  compile_expr (ctx, &vec->elements[2]);

                  struct SymbolInfo *info
                      = symbol_make_local_var (symbol_name, 0, name_part);
                  int stack_offset
                      = symbol_table_define (ctx->sym_table, info);

                  fprintf (ctx->gds->text_file,
                           "  mov [rbp - %d], rax ; move result from rax into "
                           "its stack slot\n",
                           stack_offset);

                  fprintf (ctx->gds->text_file, "  mov rax, 0 ; Result of "
                                                "define is undefined\n");
                }
            }
          else if (name_part->type == S_TYPE_LIST)
            {
              compile_define_function (ctx, vec);
              fprintf (
                  ctx->gds->text_file,
                  "  ; Set RAX to a placeholder for function definition\n");
              fprintf (
                  ctx->gds->text_file,
                  "  mov rax, 0 ; Or a pointer to a global LVAL_UNDEFINED\n");
            }
          else
            {
              fprintf (stderr, "Error: Invalid 'define' syntax. Second "
                               "element must be a symbol or a list.\n");
              exit (EXIT_FAILURE);
            }
        }
      else if (strcmp (op_name, "if") == 0)
        {
          fprintf (stderr, "'if' special form not yet implemented.\n");
          exit (EXIT_FAILURE);
        }
    }
  else if (op_info->kind == SYM_BUILTIN_FUNC || op_info->kind == SYM_USER_FUNC)
    {
      compile_function_call (ctx, op_info, vec);
    }
  else
    {
      fprintf (stderr, "Error: Cannot call non-function '%s'.\n", op_name);
      exit (EXIT_FAILURE);
    }
}