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
        "; Define a function that adds 10 to its argument\n"
        "(define (add-ten x) (+ x 10)) \n\n"
        "; Call the function with the value 5\n"
        "(define result (add-ten 5))"
  ;
  // clang-format on
  struct ParserContext parser = parser_make (source_code);
  struct ExprVector ast = parse_program (&parser);

  printf ("--- AST for new test case ---\n");
  pretty_print_ast (&ast);

  compile_program (&ast, "lisp_out");
  exprvector_cleanup (&ast);

  printf ("\nCompilation successful. To run:\n");
  printf ("  nasm -f elf64 lisp_out.s\n");
  printf ("  gcc -no-pie lisp_out.o runtime.c -o program\n");
  printf ("  ./program\n");

  return 0;
}
