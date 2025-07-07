#ifndef LISPVALUE_H
#define LISPVALUE_H

struct LispValue
{
  enum LispValueType
  {
    LVAL_NUM,  // A floating-point number (double)
    LVAL_SYM,  // A symbol (pointer to char* name)
    LVAL_STR,  // A string literal (pointer to char* content)
    LVAL_PAIR, // A cons cell: (car . cdr) - 2 pointers to other LispValue*
    LVAL_FUNC, // A compiled Lisp function (pointer to assembly code block)
    LVAL_BUILTIN,  // pointer to C function in runtime
    LVAL_NIL,      // empty list '()', also represents #f (false)
    LVAL_TRUE,     // The boolean #t (true)
    LVAL_UNDEFINED // For uninitialized variables, etc.
  } type;

  union LispValueData
  {
    double num_val;
    char *str_val;
    struct
    {
      struct LispValue *car;
      struct LispValue *cdr;
    } pair_val;
    void *func_ptr;
  } value;

  // void* gc_metadata;
};

#define LISPVALUE_TYPE_OFFSET 0
#define LISPVALUE_VALUE_OFFSET 8
#define LISPVALUE_PAIR_CAR_OFFSET (LISPVALUE_VALUE_OFFSET + 0)
#define LISPVALUE_PAIR_CDR_OFFSET (LISPVALUE_VALUE_OFFSET + 8)

#endif