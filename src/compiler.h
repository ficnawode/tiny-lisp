
#ifndef COMPILER_H
#define COMPILER_H
#include "expr.h"
void compile_program (struct ExprVector *program, const char *output_filename);
#endif