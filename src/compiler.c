#include "compiler.h"
#include "expr.h"
#include "global_data_sections.h"
#include "lispvalue.h"
#include "scope.h"
#include "symbol.h"

#include <ctype.h> // For isalnum
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Forward Declarations & Compiler Context ---

// Using `struct` keyword directly, no typedef.
struct CompilerContext
{
  struct SymbolTable *sym_table;
  struct GlobalDataSections *gds;
  FILE *text_section; // Convenience pointer to the main code section
};

static void compile_expr (struct CompilerContext *ctx, struct Expr *expr);
static void compile_atom (struct CompilerContext *ctx, struct Atom *atom);
static void compile_list (struct CompilerContext *ctx, struct Expr *list_expr);

// --- Helper Functions ---

/**
 * @brief Creates a new, unique label ID for things like if/else blocks.
 */
static int
new_label_id ()
{
  static int label_counter = 0;
  return label_counter++;
}

/**
 * @brief Sanitizes a symbol name to be a valid assembly label.
 *        Replaces any non-alphanumeric characters with underscores.
 * @param buffer The output buffer for the sanitized label.
 * @param buf_size The size of the output buffer.
 * @param prefix A prefix for the label (e.g., "G_" for globals).
 * @param name The original symbol name.
 */
static void
sanitize_label (char *buffer, size_t buf_size, const char *prefix,
                const char *name)
{
  size_t prefix_len = strlen (prefix);
  // Ensure buffer has space for prefix, name, and null terminator
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

/**
 * @brief Populates the global scope with built-in functions and special forms.
 */
static void
populate_global_scope (struct SymbolTable *st)
{
  struct SymbolInfo *define_info = symbol_make_special_form ("define", NULL);
  symbol_map_emplace (st->global_scope->symbol_map, "define", define_info);

  struct SymbolInfo *if_info = symbol_make_special_form ("if", NULL);
  symbol_map_emplace (st->global_scope->symbol_map, "if", if_info);

  struct SymbolInfo *add_info = symbol_make_builtin_func ("+", NULL, NULL);
  symbol_map_emplace (st->global_scope->symbol_map, "+", add_info);
}

// --- Main Compilation Driver ---

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

  struct CompilerContext ctx
      = { .sym_table = sym_table, .gds = gds, .text_section = gds->text_file };

  if (!ctx.text_section)
    {
      fprintf (stderr, "Fatal: Could not get .text section file.\n");
      gds_close_and_finalize (gds);
      symbol_table_destroy (sym_table);
      exit (EXIT_FAILURE);
    }

  // --- Generate File Header ---
  fprintf (ctx.text_section, "global main\n");
  fprintf (ctx.text_section, "extern lisp_add\n");
  fprintf (ctx.text_section, "extern lisp_make_number\n\n");

  // --- Generate Main Function ---
  fprintf (ctx.text_section, "main:\n");
  fprintf (ctx.text_section, "  push rbp\n");
  fprintf (ctx.text_section, "  mov rbp, rsp\n\n");

  // Compile all top-level expressions
  for (size_t i = 0; i < program->len; ++i)
    {
      compile_expr (&ctx, &program->elements[i]);
    }

  // --- Generate Main Function Epilogue ---
  fprintf (ctx.text_section, "\n  ; Main function epilogue\n");
  fprintf (ctx.text_section, "  mov rax, 0      ; Return 0 for success\n");
  fprintf (ctx.text_section, "  mov rsp, rbp\n");
  fprintf (ctx.text_section, "  pop rbp\n");
  fprintf (ctx.text_section, "  ret\n");

  gds_close_and_finalize (gds);
  symbol_table_destroy (sym_table);
}

// --- Expression-level Compilation ---

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

        fprintf (ctx->text_section,
                 "\n  ; Create LispValue for number %.2f on the heap\n",
                 atom->value.number);
        fprintf (ctx->text_section, "  movsd xmm0, [rel L_double_%d]\n",
                 label_id);
        fprintf (ctx->text_section, "  call lisp_make_number\n");
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

        fprintf (ctx->text_section, "\n  ; Load symbol '%s'\n",
                 atom->value.symbol);
        switch (info->kind)
          {
          case SYM_LOCAL_VAR:
            fprintf (ctx->text_section, "  mov rax, [rbp - %d]\n",
                     info->location.stack_offset);
            break;
          case SYM_GLOBAL_VAR:
            fprintf (ctx->text_section, "  mov rax, [%s]\n",
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

  struct SymbolInfo *op_info
      = symbol_table_lookup (ctx->sym_table, first->val.atom_val.value.symbol);
  if (!op_info)
    {
      fprintf (stderr, "Error: Undefined operator '%s'\n",
               first->val.atom_val.value.symbol);
      exit (EXIT_FAILURE);
    }

  if (op_info->kind == SYM_SPECIAL_FORM)
    {
      if (strcmp (op_info->name, "define") == 0)
        {
          if (vec->len != 3 || vec->elements[1].type != S_TYPE_ATOM)
            {
              fprintf (stderr, "Error: Invalid 'define' syntax.\n");
              exit (EXIT_FAILURE);
            }
          char *symbol_name = vec->elements[1].val.atom_val.value.symbol;

          compile_expr (ctx, &vec->elements[2]);

          fprintf (ctx->text_section, "  push rax\n");

          if (ctx->sym_table->current_scope == ctx->sym_table->global_scope)
            {
              FILE *data = ctx->gds->data_file;

              char label_buf[256];
              sanitize_label (label_buf, sizeof (label_buf), "G_",
                              symbol_name);

              char *label_name = strdup (label_buf);
              if (!label_name)
                {
                  perror ("strdup failed");
                  exit (EXIT_FAILURE);
                }
              fprintf (data, "%s: dq 0\n", label_name);

              struct SymbolInfo *info = symbol_make_global_var (
                  symbol_name, symbol_name, &vec->elements[1]);
              info->location.global_asm_label = label_name;
              symbol_table_define (ctx->sym_table, info);

              fprintf (ctx->text_section, "  pop rbx\n");
              fprintf (ctx->text_section, "  mov [%s], rbx\n", label_name);
              fprintf (ctx->text_section, "  mov rax, rbx\n");
            }
          else
            {
              fprintf (stderr, "Local 'define' is not yet supported.\n");
              exit (EXIT_FAILURE);
            }
        }
      else if (strcmp (op_info->name, "if") == 0)
        {
          fprintf (
              stderr,
              "'if' special form not fully implemented in this refactor.\n");
          exit (EXIT_FAILURE);
        }
    }
  else if (op_info->kind == SYM_BUILTIN_FUNC || op_info->kind == SYM_USER_FUNC)
    {
      if (strcmp (op_info->name, "+") == 0)
        {
          if (vec->len - 1 != 2)
            {
              fprintf (stderr, "Error: Built-in '+' requires 2 arguments.\n");
              exit (EXIT_FAILURE);
            }

          fprintf (ctx->text_section, "\n  ; ABI-compliant call to '%s'\n",
                   op_info->name);
          compile_expr (ctx, &vec->elements[2]);
          fprintf (ctx->text_section, "  push rax\n");

          compile_expr (ctx, &vec->elements[1]);

          fprintf (ctx->text_section, "  mov rdi, rax\n");
          fprintf (ctx->text_section, "  pop rsi\n");

          fprintf (ctx->text_section, "  call lisp_add\n");
        }
      else
        {
          fprintf (stderr, "Function call to '%s' is not implemented.\n",
                   op_info->name);
          exit (EXIT_FAILURE);
        }
    }
  else
    {
      fprintf (stderr, "Error: Cannot call non-function '%s'.\n",
               op_info->name);
      exit (EXIT_FAILURE);
    }
}