;; -*- coding: utf-8 -*-

(import (scheme base) (scheme char) (scheme lazy)
        (scheme inexact) (scheme complex)
        (scheme time)
        (scheme file) (scheme read) (scheme write)
        (scheme eval) (scheme process-context) (scheme case-lambda)
        (only (scheme r5rs) scheme-report-environment null-environment)
        (chibi test)  ; or (srfi 64)
        )


(test-begin "R7RS")
(test-begin "read byte file")


(test 52  ;; "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
	  (let ((handle (open-binary-input-file "input.txt")))
		(let ((buffer (read-bytevector 512 handle)))
		  (close-input-port handle)
          (display "<") (display buffer) (display ">\n")
		  52)))


(test-end)

(test-end)
