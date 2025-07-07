;; test_if.lisp
;; An updated program to test the 'if' special form with true booleans.

;; Test 1: A basic if-then-else with a #t condition.
;; The 'then' branch (100) should be executed.
;; The final value in RAX after this expression should be a LispValue pointer for 100.
(define true-result (if #t 100 200))


;; Test 2: A basic if-then-else with a #f condition.
;; The 'else' branch (999) should be executed.
;; The final value in RAX should be a LispValue pointer for 999.
(define false-result (if #f 888 999))


;; Test 3: An if-then without an else, with a #f condition.
;; The 'then' branch should be skipped.
;; The missing 'else' branch should result in the #f value (a pointer to G_LISP_NIL).
(if #f 400)


;; Test 4: Nested if statements.
;; This tests that labels for inner and outer ifs don't conflict.
;; The outer 'if' condition is #t, so it evaluates the inner 'if'.
;; The inner 'if' condition is #f, so it evaluates to 600.
;; The final result in RAX should be a LispValue for 600.
(if #t
    (if #f 500 600)
    700)