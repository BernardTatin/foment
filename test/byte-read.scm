;; -*- coding: utf-8 -*-

(import (scheme base) (scheme char) (scheme lazy)
		(scheme inexact) (scheme complex)
		(scheme time)
		(scheme file) (scheme read) (scheme write)
		(scheme eval) (scheme process-context) (scheme case-lambda)
		(only (scheme r5rs) scheme-report-environment null-environment)
		(chibi test)  ; or (srfi 64)
		)


(test-begin "read byte file")

(define-syntax rb-test
  (syntax-rules ()
				((rb-test expected-len bytes-to-read)
				 (test expected-len  ;; "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
					   (let ((handle (open-binary-input-file "input.txt")))
						 (display "\n\n=================================\n")
						 (let ((buffer (read-bytevector bytes-to-read handle)))
						   (close-input-port handle)
						   (display "<") (display buffer) (display ">\n")
						   (if (eof-object? buffer)
							 buffer
							 (bytevector-length buffer))))))))


(rb-test 3 3)
(rb-test 25 25)
(rb-test 52 52)
(rb-test 52 128)

(test-end)
