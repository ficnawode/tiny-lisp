#include "lexer.h"

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
        "(* my-symbol 'another-symbol? 123foo bar-baz!)\n" // 123foo should be
                                                           // symbol
        "(error-test #invalid-char)"; // # should be an error

  struct LexerContext lexer = lexer_init (source_code);

  printf ("Lexing Lisp source code:\n---\n%s\n---\n", source_code);

  struct Token token;
  do
    {
      token = lexer_next (&lexer);
      print_token (&token);
      free_token (&token);
    }
  while (token.type != TOKEN_EOF && token.type != TOKEN_ERROR);

  // If an error occurred, scan one more time to demonstrate it moves past it
  // (A real parser might stop or have more complex recovery)
  if (token.type == TOKEN_ERROR && lexer.source[lexer.current_pos] != '\0')
    {
      printf ("\n--- Continuing after error for demonstration ---\n");
      // The previous error token already advanced past the problematic char,
      // so the next call should pick up from there.
      do
        {
          token = lexer_next (&lexer);
          print_token (&token);
          free_token (&token);
        }
      while (token.type != TOKEN_EOF && token.type != TOKEN_ERROR);
    }

  printf ("\n--- Test for malformed number (123.A) ---\n");
  lexer
      = lexer_init ("123.A"); // This should be a symbol according to our logic
  token = lexer_next (&lexer);
  print_token (&token);
  free_token (&token);
  token = lexer_next (&lexer); // EOF
  print_token (&token);
  free_token (&token);
  lexer_cleanup (&lexer);

  printf ("\n--- Test for just a dot (should be symbol) ---\n");
  lexer = lexer_init (".");
  token = lexer_next (&lexer);
  print_token (&token);
  free_token (&token);
  token = lexer_next (&lexer); // EOF
  print_token (&token);
  free_token (&token);
  lexer_cleanup (&lexer);

  printf ("\n--- Test for just + or - (should be symbol) ---\n");
  lexer = lexer_init ("+-");
  token = lexer_next (&lexer);
  print_token (&token);
  free_token (&token);
  token = lexer_next (&lexer);
  print_token (&token);
  free_token (&token);
  token = lexer_next (&lexer); // EOF
  print_token (&token);
  free_token (&token);
  lexer_cleanup (&lexer);

  printf ("\n--- Test for unterminated string ---\n");
  lexer = lexer_init ("\"unclosed string");
  token = lexer_next (&lexer);
  print_token (&token);
  free_token (&token);
  token = lexer_next (&lexer); // Should be EOF after error
  print_token (&token);
  free_token (&token);
  lexer_cleanup (&lexer);

  lexer_cleanup (&lexer); // Final cleanup if not already done

  return 0;
}
