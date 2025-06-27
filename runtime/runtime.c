#include "lispvalue.h"
#include <stdio.h>  // For printf, for debugging
#include <stdlib.h> // For malloc

// In runtime.c
struct LispValue *
lisp_make_number (double num)
{
  struct LispValue *result
      = (struct LispValue *)malloc (sizeof (struct LispValue));
  if (!result)
    {
      perror ("malloc failed in lisp_make_number");
      exit (1);
    }
  result->type = LVAL_NUM;
  result->value.num_val = num;
  return result;
}

// The function our assembly code will call.
// extern "C" is not needed in a .c file compiled with gcc.
struct LispValue *
lisp_add (struct LispValue *arg1, struct LispValue *arg2)
{
  // Allocate the new LispValue on the heap. This is the correct
  // way to handle memory that needs to outlive the function call.
  struct LispValue *result
      = (struct LispValue *)malloc (sizeof (struct LispValue));
  if (!result)
    {
      perror ("malloc failed in lisp_add");
      exit (1);
    }

  result->type = LVAL_NUM;
  result->value.num_val = arg1->value.num_val + arg2->value.num_val;

  // C functions return their pointer value in RAX by default, which is
  // exactly what our compiler's convention requires.
  return result;
}