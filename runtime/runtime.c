#include "lispvalue.h"
#include <stdio.h>
#include <stdlib.h>

struct LispValue *lisp_make_number(double num) {
  struct LispValue *result =
      (struct LispValue *)malloc(sizeof(struct LispValue));
  if (!result) {
    perror("malloc failed in lisp_make_number");
    exit(1);
  }
  result->type = LVAL_NUM;
  result->value.num_val = num;
  return result;
}

struct LispValue *lisp_add(struct LispValue *arg1, struct LispValue *arg2) {
  struct LispValue *result =
      (struct LispValue *)malloc(sizeof(struct LispValue));
  if (!result) {
    perror("malloc failed in lisp_add");
    exit(1);
  }

  result->type = LVAL_NUM;
  result->value.num_val = arg1->value.num_val + arg2->value.num_val;

  // C functions return their pointer value in RAX by default
  return result;
}

struct LispValue *lisp_subtract(struct LispValue *arg1,
                                struct LispValue *arg2) {
  struct LispValue *result =
      (struct LispValue *)malloc(sizeof(struct LispValue));
  if (!result) {
    perror("malloc failed in lisp_subtract");
    exit(1);
  }

  result->type = LVAL_NUM;
  result->value.num_val = arg1->value.num_val - arg2->value.num_val;

  return result;
}

struct LispValue *lisp_multiply(struct LispValue *arg1,
                                struct LispValue *arg2) {
  struct LispValue *result =
      (struct LispValue *)malloc(sizeof(struct LispValue));
  if (!result) {
    perror("malloc failed in lisp_multiply");
    exit(1);
  }

  result->type = LVAL_NUM;
  result->value.num_val = arg1->value.num_val * arg2->value.num_val;

  return result;
}

struct LispValue *lisp_divide(struct LispValue *arg1, struct LispValue *arg2) {
  struct LispValue *result =
      (struct LispValue *)malloc(sizeof(struct LispValue));
  if (!result) {
    perror("malloc failed in lisp_divide");
    exit(1);
  }

  result->type = LVAL_NUM;
  result->value.num_val = arg1->value.num_val / arg2->value.num_val;

  return result;
}

void lisp_debug_print(struct LispValue *arg) {
  switch (arg->type) {
  case LVAL_NUM:
    printf("Double %lf", arg->value.num_val);
    break;
  case LVAL_SYM:
    printf("Symbol; name: %s", arg->value.str_val);
    break;
  case LVAL_STR:
    printf("String: %s", arg->value.str_val);
    break;
  case LVAL_PAIR:
    printf("PAIR");
    break;
  case LVAL_FUNC:
    printf("Function pointer at %p", arg->value.func_ptr);
    break;
  case LVAL_BUILTIN:
    printf("Built-in function at %p", arg->value.func_ptr);
    break;
  case LVAL_NIL:
    printf("NIL");
    break;
  case LVAL_TRUE:
    printf("True");
    break;
  case LVAL_UNDEFINED:
    printf("Undefined");
    break;
  default:
    printf("Printing error: unknown lisp value type");
  }
}