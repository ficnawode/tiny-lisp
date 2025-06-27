#include "compiler.h"
#include "parser.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int
main (int argc, char **argv)
{
  // clang-format off
  const char *source_code = 
        "(define z (+ 5 10))"
  
  ;
  // clang-format on
  struct ParserContext parser = parser_make (source_code);
  struct ExprVector ast = parse_program (&parser);
  pretty_print_ast (&ast);
  compile_program (&ast, "lisp_out");
  exprvector_cleanup (&ast);

  return 0;
}
