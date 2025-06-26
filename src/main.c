#include "parser.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int
main (int argc, char **argv)
{
  const char *source_code
      = "(define (fib n) ; This is a recursive fibonacci function\n"
        "  (if (< n 2)\n"
        "      n\n"
        "      (+ (fib (- n 1))\n"
        "         (fib (- n 2)))))\n"
        "\n"
        "(display \"Hello, Lisp!\")\n"
        "(+ 10 -5 3.14 -0.001)\n"
        "(* my-symbol 'another-symbol? 123foo bar-baz!)\n"; // 123foo should be
                                                            // symbol

  struct ParserContext parser = parser_init (source_code);
  struct ExprList *ast = parse_program (&parser);
  pretty_print_ast (ast);
  return 0;
}
