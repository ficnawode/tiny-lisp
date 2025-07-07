
#ifndef COMPILER_H
#define COMPILER_H
#include "expr.h"
#include <stdio.h>

struct CompilerContext
{
  struct SymbolTable *sym_table;
  struct GlobalDataSections *gds;
};

void compile_program (struct ExprVector *program, const char *output_filename);
#endif