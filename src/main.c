#include "compiler.h"
#include "parser.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *
read_file_to_string (const char *filename)
{
  FILE *file = fopen (filename, "rb");
  if (!file)
    {
      perror ("Error opening file");
      return NULL;
    }

  fseek (file, 0, SEEK_END);
  long length = ftell (file);
  fseek (file, 0, SEEK_SET);

  if (length == -1L)
    {
      perror ("ftell failed");
      fclose (file);
      return NULL;
    }

  char *buffer = malloc (length + 1);
  if (!buffer)
    {
      fprintf (stderr, "Could not allocate memory for file buffer\n");
      fclose (file);
      return NULL;
    }

  if (fread (buffer, 1, length, file) != (size_t)length)
    {
      fprintf (stderr, "Failed to read entire file into buffer\n");
      free (buffer);
      fclose (file);
      return NULL;
    }

  buffer[length] = '\0';
  fclose (file);
  return buffer;
}

int
main (int argc, char **argv)
{
  if (argc < 2)
    {
      fprintf (stderr, "Usage: %s <input.lisp>\n", argv[0]);
      return EXIT_FAILURE;
    }

  const char *input_filename = argv[1];
  char *source_code = read_file_to_string (input_filename);
  if (!source_code)
    {
      return EXIT_FAILURE;
    }

  char output_basename[256];
  strncpy (output_basename, input_filename, sizeof (output_basename) - 1);
  output_basename[sizeof (output_basename) - 1] = '\0';
  char *dot = strrchr (output_basename, '.');
  if (dot != NULL)
    {
      *dot = '\0'; // Truncate at the last dot to get the base name
    }

  struct ParserContext parser = parser_make (source_code);
  struct ExprVector ast = parse_program (&parser);

  printf ("--- AST for %s ---\n", input_filename);
  pretty_print_ast (&ast);

  compile_program (&ast, output_basename);

  exprvector_cleanup (&ast);
  free (source_code);

  printf ("\nCompilation successful. To run:\n");
  printf ("  nasm -f elf64 %s.s\n", output_basename);
  printf ("  gcc -no-pie %s.o runtime.c -o %s\n", output_basename,
          output_basename);
  printf ("  ./%s\n", output_basename);

  return 0;
}