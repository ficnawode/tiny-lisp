# Tiny Lisp

Compiler: subset of lisp to assembly (x86-64) 

Implemented in pure C. 

Note: This project is not meant for serious use and only meant for fun/educational purposes (we physicists do not have compilers classes unfortunately).

# Use
To compile the compiler you will need `make`, `nasm`, and `gcc` (and `gdb` if you want to see anything interesting).

```
make
```
.

To compile a lisp program feed the lisp code file into the binary, for instance:

```
./bin/a.out lisp/test_locals.lisp
```

This will produce an assembly file, in this case `test_locals.s`. You must then compile the assembly into an object (`.o`) file, for example:

```
nasm -f elf64 -g lisp/test_if_else.s -o lisp/test_if_else.o
```

The object file must finally be linked to the runtime library object. I use `gcc`:

```
gcc lisp/test_if_else.o obj/runtime.o -o lisp/test_if_else.out 
```

To launch the fully compiled program simply
```
./lisp/test_if_else.out
``` 

If everything worked out, you will probably see... nothing! Printing to standard output is not implemented yet, but you may probe inside the program using gdb, namely:

```
gdb <name of your binary>
```

If you do not know (or remember) how to use gdb, type in `help`, otherwise [here](https://web.mit.edu/gnu/doc/html/gdb_toc.html) is a guide (hint: set a breakpoint in runtime.c using `b` and then print one of the values using `p`). 

# Implemented 

- define (local and global)
- if/else statements
- basic arithmetic (+-*/)
- function definitions and calls (functions are only global for now)

# To Do

- list traversion 
- loops 
- print to stdout