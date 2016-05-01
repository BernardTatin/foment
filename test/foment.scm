;;;
;;; Foment
;;;

(import (foment base))

;; set!-values

(define sv1 1)
(define sv2 2)

(check-equal (a b c)
    (let ((sv3 3)) (set!-values (sv1 sv2 sv3) (values 'a 'b 'c)) (list sv1 sv2 sv3)))

(set!-values (sv1 sv2) (values 10 20))
(check-equal (10 20) (list sv1 sv2))

(set!-values () (values))

(check-equal #f (let () (set!-values () (values)) #f))

(check-error (assertion-violation) (let () (set!-values () (values 1))))

;; make-latin1-port
;; make-utf8-port
;; make-utf16-port

(check-error (assertion-violation make-latin1-port) (make-latin1-port))
(check-error (assertion-violation make-latin1-port) (make-latin1-port (current-input-port)))
(check-error (assertion-violation make-latin1-port) (make-latin1-port (current-output-port)))
(check-error (assertion-violation make-latin1-port)
        (make-latin1-port (open-binary-input-file "foment.scm") #t))

(define (tst-string m)
    (let ((s (make-string m)))
        (define (set-ch n m o)
            (if (< n m)
                (begin
                    (string-set! s n (integer->char (+ n o)))
                    (set-ch (+ n 1) m o))))
        (set-ch 0 m (char->integer #\!))
        s))

(define (tst-read-port port)
    (define (read-port port n)
        (let ((obj (read-char port)))
            (if (not (eof-object? obj))
                (begin
                    (if (or (not (char? obj)) (not (= n (char->integer obj))))
                        (error "expected character" (integer->char n) obj))
                    (read-port port (+ n 1))))))
    (read-port port (char->integer #\!)))

(call-with-port (make-utf8-port (open-binary-output-file "output.utf8"))
    (lambda (port)
        (display (tst-string 5000) port)))

(call-with-port (make-utf8-port (open-binary-input-file "output.utf8")) tst-read-port)

(check-error (assertion-violation make-utf8-port) (make-utf8-port))
(check-error (assertion-violation make-utf8-port) (make-utf8-port (current-input-port)))
(check-error (assertion-violation make-utf8-port) (make-utf8-port (current-output-port)))
(check-error (assertion-violation make-utf8-port)
        (make-utf8-port (open-binary-input-file "foment.scm") #t))

(call-with-port (make-utf16-port (open-binary-output-file "output.utf16"))
    (lambda (port)
        (display (tst-string 5000) port)))

(call-with-port (make-utf16-port (open-binary-input-file "output.utf16")) tst-read-port)

(check-error (assertion-violation make-utf16-port) (make-utf16-port))
(check-error (assertion-violation make-utf16-port) (make-utf16-port (current-input-port)))
(check-error (assertion-violation make-utf16-port) (make-utf16-port (current-output-port)))
(check-error (assertion-violation make-utf16-port)
        (make-utf16-port (open-binary-input-file "foment.scm") #t))

;; with-continuation-mark
;; current-continuation-marks

(define (tst)
    (with-continuation-mark 'a 1
        (with-continuation-mark 'b 2
            (let ((ret (with-continuation-mark 'c 3 (current-continuation-marks))))
                ret))))

(check-equal (((c . 3)) ((b . 2) (a . 1))) (let ((ret (tst))) ret))

(define (count n m)
    (if (= n m)
        (current-continuation-marks)
        (let ((r (with-continuation-mark 'key n (count (+ n 1) m))))
            r)))

(check-equal (((key . 3)) ((key . 2)) ((key . 1)) ((key . 0))) (count 0 4))

;; call-with-continuation-prompt
;; abort-current-continuation
;; default-prompt-tag
;; default-prompt-handler

(define rl '())
(define (st o)
    (set! rl (cons o rl)))

(define (at1)
    (call-with-continuation-prompt
        (lambda (x y) (st x) (st y)
            (dynamic-wind
                (lambda () (st 'before))
                (lambda ()
                    (st 'thunk)
                    (abort-current-continuation 'prompt-tag 'a 'b 'c)
                    (st 'oops))
                (lambda () (st 'after))))
        'prompt-tag
        (lambda (a b c) (st a) (st b) (st c))
        'x 'y)
    (reverse rl))

(check-equal (x y before thunk after a b c) (at1))

(set! rl '())
(define (at2)
    (call-with-continuation-prompt
        (lambda () (st 'proc)
            (dynamic-wind
                (lambda () (st 'before))
                (lambda () (st 'thunk) (abort-current-continuation (default-prompt-tag)
                    (lambda () (st 'handler))) (st 'oops))
                (lambda () (st 'after))))
        (default-prompt-tag)
        default-prompt-handler)
    (reverse rl))

(check-equal (proc before thunk after handler) (at2))

(set! rl '())
(define (at3)
    (call-with-continuation-prompt
        (lambda () (st 'proc)
            (dynamic-wind
                (lambda () (st 'before1))
                (lambda ()
                    (st 'thunk1)
                    (dynamic-wind
                        (lambda () (st 'before2))
                        (lambda ()
                            (st 'thunk2)
                            (abort-current-continuation (default-prompt-tag)
                                (lambda () (st 'handler)))
                            (st 'oops))
                        (lambda () (st 'after2))))
                (lambda () (st 'after1))))
        (default-prompt-tag)
        default-prompt-handler)
    (reverse rl))

(check-equal (proc before1 thunk1 before2 thunk2 after2 after1 handler) (at3))

;; srfi-39
;; parameterize

(define radix (make-parameter 10))

(define boolean-parameter (make-parameter #f
    (lambda (x)
        (if (boolean? x)
            x
            (error "only booleans are accepted by boolean-parameter")))))

(check-equal 10 (radix))
(radix 2)
(check-equal 2 (radix))
(check-error (assertion-violation error) (boolean-parameter 0))

(check-equal 16 (parameterize ((radix 16)) (radix)))
(check-equal 2 (radix))

(define prompt
    (make-parameter 123
        (lambda (x)
            (if (string? x)
                x
                (number->string x 10)))))

(check-equal "123" (prompt))
(prompt ">")
(check-equal ">" (prompt))

(define (f n) (number->string n (radix)))

(check-equal "1010" (f 10))
(check-equal "12" (parameterize ((radix 8)) (f 10)))
(check-equal "1010" (parameterize ((radix 8) (prompt (f 10))) (prompt)))

(define p1 (make-parameter 10))
(define p2 (make-parameter 20))

(check-equal 10 (p1))
(check-equal 20 (p2))
(p1 100)
(check-equal 100 (p1))

(check-equal 1000 (parameterize ((p1 1000) (p2 200)) (p1)))
(check-equal 100 (p1))
(check-equal 20 (p2))
(check-equal 1000 (parameterize ((p1 10) (p2 200)) (p1 1000) (p1)))
(check-equal 100 (p1))
(check-equal 20 (p2))
(check-equal 1000 (parameterize ((p1 0)) (p1) (parameterize ((p2 200)) (p1 1000) (p1))))

(define *k* #f)
(define p (make-parameter 1))
(define (tst)
    (parameterize ((p 10))
        (if (call/cc (lambda (k) (set! *k* k) #t))
            (p 100)
            #f)
        (p)))

(check-equal 1 (p))
(tst)
;(check-equal 100 (tst))
(check-equal 1 (p))
(check-equal 10 (*k* #f))
(check-equal 1 (p))

(define *k2* #f)
(define p2 (make-parameter 2))
(define (tst2)
    (parameterize ((p2 20))
        (call/cc (lambda (k) (set! *k2* k)))
        (p2)))

(check-equal 2 (p2))
(check-equal 20 (tst2))
(check-equal 2 (p2))
(check-equal 20 (*k2*))
(check-equal 2 (p2))

#|
;;
;; guardians
;;

(collect #t)
(collect #t)
(collect #t)
(collect #t)

(define g (make-guardian))
(check-equal #f (g))
(collect)
(check-equal #f (g))
(collect #t)
(check-equal #f (g))

(g (cons 'a 'b))
;(check-equal #f (g))
(collect)
(check-equal (a . b) (g))

(g '#(d e f))
(check-equal #f (g))
(collect)
(check-equal #(d e f) (g))

(check-equal #f (g))
(define x '#(a b c))
(define y '#(g h i))
(collect)
(collect)
(collect #t)
(check-equal #f (g))

(collect #t)
(define h (make-guardian))
(check-equal #f (h))
(g x)
(define x #f)
(h y)
(define y #f)
(check-equal #f (g))
(check-equal #f (h))
(collect)
(check-equal #f (g))
(check-equal #f (h))
(collect)
(check-equal #f (g))
(check-equal #f (h))
(collect #t)
(check-equal #(a b c) (g))
(check-equal #(g h i) (h))
(check-equal #f (g))
(check-equal #f (h))

(g "123")
(g "456")
(g "789")
(h #(1 2 3))
(h #(4 5 6))
(h #(7 8 9))
(collect)
(check-equal #t (let ((v (g))) (or (equal? v "789") (equal? v "456") (equal? v "123"))))
(check-equal #t (let ((v (g))) (or (equal? v "789") (equal? v "456") (equal? v "123"))))
(check-equal #t (let ((v (g))) (or (equal? v "789") (equal? v "456") (equal? v "123"))))
(check-equal #f (g))
(collect)
(collect #t)
(check-equal #f (g))
(check-equal #t (let ((v (h))) (or (equal? v #(1 2 3)) (equal? v #(4 5 6)) (equal? v #(7 8 9)))))
(check-equal #t (let ((v (h))) (or (equal? v #(1 2 3)) (equal? v #(4 5 6)) (equal? v #(7 8 9)))))
(check-equal #t (let ((v (h))) (or (equal? v #(1 2 3)) (equal? v #(4 5 6)) (equal? v #(7 8 9)))))
(check-equal #f (h))

; From: Guardians in a generation-based garbage collector.
; by R. Kent Dybvig, Carl Bruggeman, and David Eby.

(define G (make-guardian))
(define x (cons 'a 'b))
(G x)
(check-equal #f (G))
(set! x #f)
(collect)
(check-equal (a . b) (G))
(check-equal #f (G))

(define G (make-guardian))
(define x (cons 'a 'b))
(G x)
(G x)
(set! x #f)
(collect)
(check-equal (a . b) (G))
(check-equal (a . b) (G))
(check-equal #f (G))

(define G (make-guardian))
(define H (make-guardian))
(define x (cons 'a 'b))
(G x)
(H x)
(set! x #f)
(collect)
(check-equal (a . b) (G))
(check-equal (a . b) (H))

(define G (make-guardian))
(define H (make-guardian))
(define x (cons 'a 'b))
(G H)
(H x)
(set! x #f)
(set! H #f)
(collect)
(check-equal (a . b) ((G)))

;;
;; trackers
;;

(define t (make-tracker))
(check-equal #f (t))
(define v1 #(1 2 3))
(define v2 (cons 'a 'b))
(define r2 "(cons 'a 'b)")
(define v3 "123")
(define r3 '((a b) (c d)))
(t v1)
(t v2 r2)
(t v3 r3)
(check-equal #f (t))
(collect)
(check-equal #(1 2 3) (t))
(check-equal "(cons 'a 'b)" (t))
(check-equal ((a b) (c d)) (t))
(check-equal #f (t))
(collect)
(check-equal #f (t))
|#
;;
;; Collector and Back References
;;

(define v (make-vector (* 1024 128)))
(collect)
(collect)
(define (fill-vector! vector idx)
    (if (< idx (vector-length vector))
        (begin
            (vector-set! vector idx (cons idx idx))
            (fill-vector! vector (+ idx 1)))))
(fill-vector! v 0)

(define (make-big-list idx max lst)
    (if (< idx max)
        (make-big-list (+ idx 1) max (cons idx lst))))
(make-big-list 0 (* 1024 128) '())

;;
;; threads
;;

(define e (make-exclusive))
(define c (make-condition))
(define t (current-thread))
(check-equal #t (eq? t (current-thread)))

(run-thread
    (lambda ()
        (sleep 100)
        (enter-exclusive e)
        (set! t (current-thread))
        (leave-exclusive e)
        (condition-wake c)))

(enter-exclusive e)
(condition-wait c e)
(leave-exclusive e)

(check-equal #f (eq? t (current-thread)))
(check-equal #t (thread? t))
(check-equal #t (thread? (current-thread)))
(check-equal #f (thread? e))
(check-equal #f (thread? c))

(check-error (assertion-violation current-thread) (current-thread #t))

(check-error (assertion-violation thread?) (thread?))
(check-error (assertion-violation thread?) (thread? #t #t))

(check-error (assertion-violation run-thread) (run-thread))
(check-error (assertion-violation run-thread) (run-thread #t))
(check-error (assertion-violation run-thread) (run-thread + #t))
(check-error (assertion-violation run-thread) (run-thread (lambda () (+ 1 2 3)) #t))

(define unwound-it #f)

(run-thread
    (lambda ()
        (dynamic-wind
            (lambda () 'nothing)
            (lambda () (exit-thread #t))
            (lambda () (set! unwound-it #t)))))

(sleep 10)
(check-equal #t unwound-it)

(check-error (assertion-violation sleep) (sleep))
(check-error (assertion-violation sleep) (sleep #t))
(check-error (assertion-violation sleep) (sleep 1 #t))
(check-error (assertion-violation sleep) (sleep -1))

(check-equal #t (exclusive? e))
(check-equal #t (exclusive? (make-exclusive)))
(check-equal #f (exclusive? #t))
(check-error (assertion-violation exclusive?) (exclusive?))
(check-error (assertion-violation exclusive?) (exclusive? #t #t))

(check-error (assertion-violation make-exclusive) (make-exclusive #t))

(check-error (assertion-violation enter-exclusive) (enter-exclusive #t))
(check-error (assertion-violation enter-exclusive) (enter-exclusive))
(check-error (assertion-violation enter-exclusive) (enter-exclusive c))
(check-error (assertion-violation enter-exclusive) (enter-exclusive e #t))

(check-error (assertion-violation leave-exclusive) (leave-exclusive #t))
(check-error (assertion-violation leave-exclusive) (leave-exclusive))
(check-error (assertion-violation leave-exclusive) (leave-exclusive c))
(check-error (assertion-violation leave-exclusive) (leave-exclusive e #t))

(check-error (assertion-violation try-exclusive) (try-exclusive #t))
(check-error (assertion-violation try-exclusive) (try-exclusive))
(check-error (assertion-violation try-exclusive) (try-exclusive c))
(check-error (assertion-violation try-exclusive) (try-exclusive e #t))

(define te (make-exclusive))
(check-equal #t (try-exclusive te))
(leave-exclusive te)

(run-thread (lambda () (enter-exclusive te) (sleep 1000) (leave-exclusive te)))
(sleep 100)

(check-equal #f (try-exclusive te))

(check-equal #t (condition? c))
(check-equal #t (condition? (make-condition)))
(check-equal #f (condition? #t))
(check-error (assertion-violation condition?) (condition?))
(check-error (assertion-violation condition?) (condition? #t #t))

(check-error (assertion-violation make-condition) (make-condition #t))

(check-error (assertion-violation condition-wait) (condition-wait #t))
(check-error (assertion-violation condition-wait) (condition-wait c #t))
(check-error (assertion-violation condition-wait) (condition-wait #t e))
(check-error (assertion-violation condition-wait) (condition-wait c e #t))
(check-error (assertion-violation condition-wait) (condition-wait e c))

(check-error (assertion-violation condition-wake) (condition-wake #t))
(check-error (assertion-violation condition-wake) (condition-wake c #t))
(check-error (assertion-violation condition-wake) (condition-wake e))

(check-error (assertion-violation condition-wake-all) (condition-wake-all #t))
(check-error (assertion-violation condition-wake-all) (condition-wake-all c #t))
(check-error (assertion-violation condition-wake-all) (condition-wake-all e))

;; r7rs-letrec

(define-syntax r7rs-letrec
    (syntax-rules ()
        ((r7rs-letrec ((var1 init1) ...) body ...)
            (r7rs-letrec "generate temp names" (var1 ...) () ((var1 init1) ...) body ...))
        ((r7rs-letrec "generate temp names" () (temp1 ...) ((var1 init1) ...) body ...)
            (let ((var1 (no-value)) ...)
                (let ((temp1 init1) ...)
                    (set! var1 temp1) ...
                    body ...)))
        ((r7rs-letrec "generate temp names" (x y ...) (temp ...) ((var1 init1) ...) body ...)
            (r7rs-letrec "generate temp names" (y ...) (newtemp temp ...) ((var1 init1) ...)
                    (let () body ...)))))

(check-equal #t (r7rs-letrec ((even? (lambda (n) (if (zero? n) #t (odd? (- n 1)))))
                        (odd? (lambda (n) (if (zero? n) #f (even? (- n 1))))))
                    (even? 88)))

(check-equal 0 (let ((cont #f))
        (r7rs-letrec ((x (call-with-current-continuation (lambda (c) (set! cont c) 0)))
                     (y (call-with-current-continuation (lambda (c) (set! cont c) 0))))
              (if cont
                  (let ((c cont))
                      (set! cont #f)
                      (set! x 1)
                      (set! y 1)
                      (c 0))
                  (+ x y)))))

(check-equal #t
    (r7rs-letrec ((x (call/cc list)) (y (call/cc list)))
        (cond ((procedure? x) (x (pair? y)))
            ((procedure? y) (y (pair? x))))
            (let ((x (car x)) (y (car y)))
                (and (call/cc x) (call/cc y) (call/cc x)))))

(check-equal #t
    (r7rs-letrec ((x (call-with-current-continuation (lambda (c) (list #t c)))))
        (if (car x)
            ((cadr x) (list #f (lambda () x)))
            (eq? x ((cadr x))))))

(check-syntax (syntax-violation syntax-rules) (r7rs-letrec))
(check-syntax (syntax-violation syntax-rules) (r7rs-letrec (x 2) x))
(check-syntax (syntax-violation syntax-rules) (r7rs-letrec x x))
(check-syntax (syntax-violation syntax-rules) (r7rs-letrec ((x)) x))
(check-syntax (syntax-violation syntax-rules) (r7rs-letrec ((x) 2) x))
(check-syntax (syntax-violation syntax-rules) (r7rs-letrec ((x 2) y) x))
(check-syntax (syntax-violation syntax-rules) (r7rs-letrec ((x 2) . y) x))
(check-syntax (syntax-violation let) (r7rs-letrec ((x 2) (x 3)) x))
(check-syntax (syntax-violation let) (r7rs-letrec ((x 2) (y 1) (x 3)) x))
;(check-syntax (syntax-violation syntax-rules) (r7rs-letrec ((x 2))))
(check-syntax (syntax-violation syntax-rules) (r7rs-letrec ((x 2)) . x))
(check-syntax (syntax-violation syntax-rules) (r7rs-letrec ((x 2)) y . x))
(check-syntax (syntax-violation let) (r7rs-letrec (((x y z) 2)) y x))
(check-syntax (syntax-violation let) (r7rs-letrec ((x 2) ("y" 3)) y))

;;
;; file system api
;;

(check-equal #t (= (cond-expand (windows 127) (else 122))
    (file-size "lib-a-b-c.sld")))
(check-error (assertion-violation file-size) (file-size))
(check-error (assertion-violation file-size) (file-size 'not-actually-a-filename))
(check-error (assertion-violation file-size) (file-size "not actually a filename"))
(check-error (assertion-violation file-size) (file-size "not actually" "a filename"))

(check-equal #t (file-regular? "foment.scm"))
(check-equal #f (file-regular? "lib"))
(check-equal #f (file-regular? "not actually a filename"))
(check-error (assertion-violation file-regular?) (file-regular?))
(check-error (assertion-violation file-regular?) (file-regular? 'not-a-filename))
(check-error (assertion-violation file-regular?) (file-regular? "not a filename" "just not"))

(check-equal #f (file-directory? "foment.scm"))
(check-equal #t (file-directory? "lib"))
(check-equal #f (file-directory? "not actually a filename"))
(check-error (assertion-violation file-directory?) (file-directory?))
(check-error (assertion-violation file-directory?) (file-directory? 'not-a-filename))
(check-error (assertion-violation file-directory?) (file-directory? "not a filename" "just not"))

(check-equal #t (file-readable? "foment.scm"))
(check-equal #f (file-readable? "not a file"))
(check-error (assertion-violation file-readable?) (file-readable?))
(check-error (assertion-violation file-readable?) (file-readable? 'not-a-filename))
(check-error (assertion-violation file-readable?) (file-readable? "not a filename" "just not"))

(check-equal #t (file-writable? "foment.scm"))
(check-equal #f (file-writable? "not a file"))
(check-error (assertion-violation file-writable?) (file-writable?))
(check-error (assertion-violation file-writable?) (file-writable? 'not-a-filename))
(check-error (assertion-violation file-writable?) (file-writable? "not a filename" "just not"))

(check-error (assertion-violation create-symbolic-link) (create-symbolic-link "not a filename"))
(check-error (assertion-violation create-symbolic-link) (create-symbolic-link "not a" 'filename))
(check-error (assertion-violation create-symbolic-link)
    (create-symbolic-link "not a filename" "just not" "a filename"))

(call-with-output-file "rename.me" (lambda (p) (write "all good" p) (newline p)))
(if (file-regular? "me.renamed")
    (delete-file "me.renamed"))
(check-equal #f (file-regular? "me.renamed"))
(rename-file "rename.me" "me.renamed")
(check-equal #t (file-regular? "me.renamed"))

(call-with-output-file "me.overwritten" (lambda (p) (write "all bad" p) (newline p)))
(check-equal #t (file-regular? "me.overwritten"))
(rename-file "me.renamed" "me.overwritten")
(check-equal #t (file-regular? "me.overwritten"))
(check-equal "all good" (call-with-input-file "me.overwritten" (lambda (p) (read p))))

(check-error (assertion-violation rename-file) (rename-file "not a filename"))
(check-error (assertion-violation rename-file) (rename-file "not a" 'filename))
(check-error (assertion-violation rename-file)
    (rename-file "not a filename" "just not" "a filename"))

(if (file-directory? "testdirectory")
    (delete-directory "testdirectory"))

(create-directory "testdirectory")
(check-equal #t (file-directory? "testdirectory"))
(delete-directory "testdirectory")
(check-equal #f (file-directory? "testdirectory"))

(check-error (assertion-violation create-directory) (create-directory))
(check-error (assertion-violation create-directory) (create-directory 'not-a-filename))
(check-error (assertion-violation create-directory) (create-directory "not a filename" "just not"))

(check-error (assertion-violation delete-directory) (delete-directory))
(check-error (assertion-violation delete-directory) (delete-directory 'not-a-filename))
(check-error (assertion-violation delete-directory) (delete-directory "not a filename" "just not"))

(check-equal "foment.scm" (car (member "foment.scm" (list-directory "."))))
(check-equal #f (member "not a filename" (list-directory ".")))
(check-equal "test" (car (member "test" (list-directory ".."))))
(check-error (assertion-violation list-directory) (list-directory))
(check-error (assertion-violation list-directory) (list-directory 'not-a-filename))
(check-error (assertion-violation list-directory) (list-directory "not a filename" "just not"))

(check-error (assertion-violation current-directory) (current-directory 'not-a-filename))
(check-error (assertion-violation current-directory) (current-directory "not-a-filename"))
(check-error (assertion-violation current-directory) (current-directory ".." "not a filename"))

;;
;; port positioning
;;

(define obp (open-binary-output-file "output4.txt"))
(define ibp (open-binary-input-file "input.txt"))

(check-equal #t (port-has-port-position? ibp))
(check-equal #f (port-has-port-position? (current-output-port)))
(check-equal #t (port-has-port-position? obp))
(check-equal #f (port-has-port-position? (current-input-port)))
(check-equal #t (eq? port-has-port-position? port-has-set-port-position!?))

(check-error (assertion-violation positioning-port?) (port-has-port-position?))
(check-error (assertion-violation positioning-port?) (port-has-port-position? ibp obp))
(check-error (assertion-violation positioning-port?) (port-has-port-position? 'port))

(check-equal 0 (port-position ibp))
(check-equal 97 (read-u8 ibp)) ;; #\a
(check-equal 1 (port-position ibp))
(set-port-position! ibp 25 'current)
(check-equal 26 (port-position ibp))
(set-port-position! ibp 0 'end)
(check-equal 52 (port-position ibp))

(set-port-position! ibp 22 'begin)
(check-equal 22 (port-position ibp))
(set-port-position! ibp -10 'current)
(check-equal 12 (port-position ibp))
(check-equal 109 (read-u8 ibp)) ;; #\m
(check-equal 13 (port-position ibp))

(set-port-position! ibp -1 'end)
(check-equal 90 (read-u8 ibp)) ;; #\Z
(check-equal 52 (port-position ibp))

(check-equal 0 (port-position obp))
(write-bytevector #u8(0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0) obp)
(set-port-position! obp -1 'end)
(write-u8 123 obp)
(set-port-position! obp 10 'begin)
(write-u8 34 obp)
(set-port-position! obp 1 'current)
(write-u8 56 obp)
(close-port obp)

(define obp (open-binary-input-file "output4.txt"))
(check-equal #u8(0 1 2 3 4 5 6 7 8 9 34 1 56 3 4 5 6 7 8 9 123) (read-bytevector 1024 obp))

;;
;; hash trees
;;

(define ht (make-hash-tree))
(define ht-size 10000)
(define ht-v (make-vector ht-size #f))

(define (test-ht-add n cnt max)
    (if (< n max)
        (let ((idx (random ht-size)))
            (if (not (vector-ref ht-v idx))
                (begin
                    (vector-set! ht-v idx #t)
                    (set! ht (hash-tree-set! ht idx idx))
                    (test-ht-add (+ n 1) (+ cnt 1) max))
                (test-ht-add (+ n 1) cnt max)))
        cnt))

(define (test-ht-delete n cnt max)
    (if (< n max)
        (let ((idx (random ht-size)))
            (if (vector-ref ht-v idx)
                (begin
                    (vector-set! ht-v idx #f)
                    (set! ht (hash-tree-delete ht idx))
                    (test-ht-delete (+ n 1) (+ cnt 1) max))
                (test-ht-delete (+ n 1) cnt max)))
        cnt))

(define (test-ht idx fnd)
    (if (< idx ht-size)
        (begin
            (if (vector-ref ht-v idx)
                (if (not (eq? (hash-tree-ref ht idx 'fail) idx))
                    (begin
                        (display "failed: (hash-tree-ref ht idx 'fail) got: ")
                        (display (hash-tree-ref ht idx 'fail))
                        (display " expected: ")
                        (display idx)
                        (newline))))
            (test-ht (+ idx 1) (if (vector-ref ht-v idx) (+ fnd 1) fnd)))
        fnd))

(test-ht-add 0 0 (+ (random 3000) 3000))
(test-ht 0 0)
(test-ht-delete 0 0 6000)
(test-ht 0 0)

;;
;; eq hash maps
;;

(define eq-hmap (make-eq-hash-map))

(define eq-hmap-size 10000)
(define eq-hmap-v (make-vector eq-hmap-size #f))

(define (test-hmap-add n cnt max)
    (if (< n max)
        (let* ((idx (random eq-hmap-size))
                (key (string->symbol (number->string idx))))
            (if (not (vector-ref eq-hmap-v idx))
                (begin
                    (vector-set! eq-hmap-v idx #t)
                    (eq-hash-map-set! eq-hmap key idx)
                    (test-hmap-add (+ n 1) (+ cnt 1) max))
                (test-hmap-add (+ n 1) cnt max)))
        cnt))

(define (test-hmap-delete n cnt max)
    (if (< n max)
        (let* ((idx (random eq-hmap-size))
                (key (string->symbol (number->string idx))))
            (if (vector-ref eq-hmap-v idx)
                (begin
                    (vector-set! eq-hmap-v idx #f)
                    (eq-hash-map-delete eq-hmap key)
                    (test-hmap-delete (+ n 1) (+ cnt 1) max))
                (test-hmap-delete (+ n 1) cnt max)))
        cnt))

(define (test-hmap-set1 n lst max)
    (if (< n max)
        (let* ((idx (random eq-hmap-size))
                (key (string->symbol (number->string idx))))
            (if (not (vector-ref eq-hmap-v idx))
                (begin
                    (vector-set! eq-hmap-v idx #t)
                    (eq-hash-map-set! eq-hmap key #t)
                    (test-hmap-set1 (+ n 1) (cons idx lst) max))
                (test-hmap-set1 (+ n 1) lst max)))
        lst))

(define (test-hmap-set2 lst cnt)
    (if (pair? lst)
        (let* ((idx (car lst))
                (key (string->symbol (number->string idx))))
            (eq-hash-map-set! eq-hmap key idx)
            (test-hmap-set2 (cdr lst) (+ cnt 1)))
        cnt))

(define (test-hmap idx fnd)
    (if (< idx eq-hmap-size)
        (begin
            (if (vector-ref eq-hmap-v idx)
                (let ((key (string->symbol (number->string idx))))
                    (if (not (eq? (eq-hash-map-ref eq-hmap key 'fail) idx))
                        (begin
                            (display "failed: (eq-hash-map-ref eq-hmap key 'fail) got: ")
                            (display (eq-hash-map-ref eq-hmap key 'fail))
                            (display " expected: ")
                            (display idx)
                            (newline)))))
            (test-hmap (+ idx 1) (if (vector-ref eq-hmap-v idx) (+ fnd 1) fnd)))
        fnd))

(test-hmap-add 0 0 (+ (random 3000) 3000))
(test-hmap 0 0)
(test-hmap-delete 0 0 6000)
(test-hmap 0 0)
(test-hmap-set2 (test-hmap-set1 0 '() 3000) 0)
(test-hmap 0 0)

;;
;; hash maps
;;

(define hmap (make-hash-map))

(define hmap-size 10000)
(define hmap-v (make-vector hmap-size #f))

(define (test-hmap-add n cnt max)
    (if (< n max)
        (let* ((idx (random hmap-size))
                (key (string->symbol (number->string idx))))
            (if (not (vector-ref hmap-v idx))
                (begin
                    (vector-set! hmap-v idx #t)
                    (hash-map-set! hmap key idx)
                    (test-hmap-add (+ n 1) (+ cnt 1) max))
                (test-hmap-add (+ n 1) cnt max)))
        cnt))

(define (test-hmap-delete n cnt max)
    (if (< n max)
        (let* ((idx (random hmap-size))
                (key (string->symbol (number->string idx))))
            (if (vector-ref hmap-v idx)
                (begin
                    (vector-set! hmap-v idx #f)
                    (hash-map-delete! hmap key)
                    (test-hmap-delete (+ n 1) (+ cnt 1) max))
                (test-hmap-delete (+ n 1) cnt max)))
        cnt))

(define (test-hmap-set1 n lst max)
    (if (< n max)
        (let* ((idx (random hmap-size))
                (key (string->symbol (number->string idx))))
            (if (not (vector-ref hmap-v idx))
                (begin
                    (vector-set! hmap-v idx #t)
                    (hash-map-set! hmap key #t)
                    (hash-map-set! hmap key idx)
                    (test-hmap-set1 (+ n 1) (cons idx lst) max))
                (test-hmap-set1 (+ n 1) lst max)))
        lst))

(define (test-hmap-set2 lst cnt)
    (if (pair? lst)
        (let* ((idx (car lst))
                (key (string->symbol (number->string idx))))
            (hash-map-set! hmap key idx)
            (test-hmap-set2 (cdr lst) (+ cnt 1)))
        cnt))

(define (test-hmap idx fnd)
    (if (< idx hmap-size)
        (begin
            (if (vector-ref hmap-v idx)
                (let ((key (string->symbol (number->string idx))))
                    (if (not (eq? (hash-map-ref hmap key 'fail) idx))
                        (begin
                            (display "failed: (hash-map-ref hmap key 'fail) got: ")
                            (display (hash-map-ref hmap key 'fail))
                            (display " expected: ")
                            (display idx)
                            (newline)))))
            (test-hmap (+ idx 1) (if (vector-ref hmap-v idx) (+ fnd 1) fnd)))
        fnd))

(test-hmap-add 0 0 (+ (random 3000) 3000))
(check-equal #t (= (test-hmap 0 0) (hash-map-size hmap)))
(set! hmap (alist->hash-map (hash-map->alist hmap)))
(test-hmap-delete 0 0 6000)
(test-hmap-set2 (test-hmap-set1 0 '() 3000) 0)
(check-equal #t (= (test-hmap 0 0) (hash-map-size hmap)))
(check-equal #t (= (vector-length (hash-map-keys hmap)) (hash-map-size hmap)))

(set! hmap (hash-map-copy hmap))
(check-equal #t (= (test-hmap 0 0) (hash-map-size hmap)))

(check-equal #t (hash-map? hmap))
(check-equal #t (hash-map? eq-hmap))
(check-equal #f (hash-map? #(1 2 3)))
(check-equal #f (hash-map? '(1 2 3)))

;;
;; eq hash sets
;;

(define eq-hset (make-eq-hash-set))

(check-error (assertion-violation make-eq-hash-set) (make-eq-hash-set 123))

(check-equal #t (eq-hash-set? eq-hset))
(check-equal #t (hash-set? eq-hset))
(check-equal #f (eq-hash-set? (make-eq-hash-map)))
(check-equal #f (eq-hash-set? (make-hash-map)))

(check-error (assertion-violation eq-hash-set?) (eq-hash-set?))
(check-error (assertion-violation eq-hash-set?) (eq-hash-set? 123 456))

(check-equal #f (eq-hash-set-contains? eq-hset 123))
(eq-hash-set-adjoin! eq-hset 123)
(check-equal #t (eq-hash-set-contains? eq-hset 123))

(define p (cons 1 2))
(check-equal #f (eq-hash-set-contains? eq-hset p))
(eq-hash-set-adjoin! eq-hset p)
(check-equal #t (eq-hash-set-contains? eq-hset p))
(check-equal #f (eq-hash-set-contains? eq-hset (cons 1 2)))

(define s "abcdefg")
(check-equal #f (eq-hash-set-contains? eq-hset s))
(eq-hash-set-adjoin! eq-hset s)
(check-equal #t (eq-hash-set-contains? eq-hset s))
(check-equal #f (eq-hash-set-contains? eq-hset "abcdefg"))

(eq-hash-set-adjoin! eq-hset)
(eq-hash-set-adjoin! eq-hset 'abc 'def 'ghi 'jkl)
(check-equal #t (eq-hash-set-contains? eq-hset 'abc))
(check-equal #t (eq-hash-set-contains? eq-hset 'def))
(check-equal #t (eq-hash-set-contains? eq-hset 'ghi))
(check-equal #t (eq-hash-set-contains? eq-hset 'jkl))

(eq-hash-set-delete! eq-hset)
(check-equal #t (eq-hash-set-contains? eq-hset 123))
(eq-hash-set-delete! eq-hset 123)
(check-equal #f (eq-hash-set-contains? eq-hset 123))

(eq-hash-set-delete! eq-hset 'abc 'def 'ghi 'jkl)
(check-equal #f (eq-hash-set-contains? eq-hset 'abc))
(check-equal #f (eq-hash-set-contains? eq-hset 'def))
(check-equal #f (eq-hash-set-contains? eq-hset 'ghi))
(check-equal #f (eq-hash-set-contains? eq-hset 'jkl))

(check-error (assertion-violation eq-hash-set-contains?) (eq-hash-set-contains? eq-hset))
(check-error (assertion-violation eq-hash-set-contains?) (eq-hash-set-contains? 123 456))
(check-error (assertion-violation eq-hash-set-contains?) (eq-hash-set-contains? eq-hset 123 456))
(check-error (assertion-violation eq-hash-set-adjoin!) (eq-hash-set-adjoin!))
(check-error (assertion-violation eq-hash-set-adjoin!) (eq-hash-set-adjoin! 123))
(check-error (assertion-violation eq-hash-set-delete!) (eq-hash-set-delete!))
(check-error (assertion-violation eq-hash-set-delete!) (eq-hash-set-delete! 123))
