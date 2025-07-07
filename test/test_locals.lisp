; test_locals.lisp

; Define a function that uses a parameter and defines local variables.
(define (test-locals a)
  (define b 20)         ; Define a local variable 'b'
  (define c (+ a 5))    ; Define another local 'c' based on the parameter 'a'
  (+ b c)               ; Return the sum of the two locals
)

; Call the function with an argument and store the result in a global variable.
(define result (test-locals 10))