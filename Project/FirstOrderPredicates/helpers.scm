#lang racket

(provide read_expressions)
(provide build_trees)
(provide (struct-out Node))
(provide operators)
(provide is-leaf?)
(provide is_quantifier?)
(provide dimension_arb)
(provide equal_arbs?)
(provide remove_not)
(provide strip_braces)
(provide find_root)
(provide is_func_node)
(provide is_pred_node)
(provide get_next_unique)

(define current_val 0)
(define get_next_unique
  (lambda (name)
    (begin
      (set! current_val (+ current_val 1))
      (string->symbol (string-append name (number->string current_val)))
      )))

(define get_formulas
  (lambda (file_name)
    (filter (lambda (str)
              (> (string-length str) 0))
            (regexp-split #rx"(\r*\n+)" (file->string file_name)))))

(define get_tokens 
  (lambda (str_formulas)
    (map (lambda (str)
           (map string->symbol
                (regexp-match* #px"[a-z]+|[A-Z]+\\w*|∀|∃|&|[\\|]|!|->|<->|\\(|[\\)]" 
                               str)))
         str_formulas)))

(define get_literals
  (lambda (tokens)
    (letrec [[replace-op
              (lambda (lst)
                (if (null? lst) 
                    '()
                    (let [[tail_result 
                           (replace-op (cdr lst))]]  
                      (case (car lst)
                        ['!  (cons 'not tail_result)]
                        ['\| (cons 'or tail_result)]
                        ['&  (cons 'and tail_result)]
                        [else (cons (car lst) tail_result)]
                        ))))
              ]]
      (map replace-op tokens))))

(define (isPred? literal)
  (if (not (list? literal))
      (if (or (equal? 'T literal)
              (equal? 'F literal))
          #f
          (let [[first_char (string-ref (symbol->string literal) 0)]]
            (and (char>=? first_char #\A)
                 (char<=? first_char #\Z))))
      (and (equal? (length literal) 2)
           (isPred? (car literal)))))

(define (isFunc? literal)
  (if (or (not (list? literal))
          (member (car literal) operators)
          (not (equal? (length literal) 2))
          (not (list? (cadr literal))))
      #f
      (let [[first_char (string-ref (symbol->string (car literal)) 0)]]
            (and (char>=? first_char #\a)
                 (char<=? first_char #\z)))))

(define is_pred_node
  (lambda (node)
    (if (list? (Node-info node))
        #f
        (let [[first_char (string-ref (symbol->string (Node-info node)) 0)]]
          (and (char>=? first_char #\A)
               (char<=? first_char #\Z)
               (is-leaf? (Node-left node))
               (equal? 'nil (Node-right node)))))))

(define is_func_node
  (lambda (node)
    (if (not (list? (Node-info node)))
        #f
        (let [[first_char (string-ref (symbol->string (car (Node-info node))) 0)]]
          (and (char>=? first_char #\a)
               (char<=? first_char #\z)
               (is-leaf? node))))))

(define extract_list
  (lambda (exp count)
    (if (null? exp) 
        '()
        (case (car exp)
          ['|)| (if (= count 0)
                    '() 
                    (cons (car exp) 
                          (extract_list (cdr exp) (- count 1))))
                ]
          ['|(| (cons (car exp) 
                      (extract_list (cdr exp) (+ count 1)))]
          [else (cons (car exp)
                      (extract_list (cdr exp) count))]
          ))))

(define parse_braces
  (lambda (exp)
    (if (null? exp)
        '()
        (if (equal? (car exp) '|(| )
            (let [(big_list (extract_list (cdr exp) 0))]
              (cons (parse_braces big_list)
                    (parse_braces (drop exp (+ (length big_list)
                                               2)))))
            (cons (car exp) (parse_braces (cdr exp)))
            ))))

(define read_expressions
  (lambda (file)
    (let [[literals (get_literals (get_tokens (get_formulas file)))]]
      (map parse_braces literals))))

(define-struct Node (info left right) #:transparent)

(define split_list
  (lambda (token lst)
    (letrec [(split (lambda (lst pre)
                   (if (equal? (car lst) token)
                       (list pre (cdr lst))
                       (split (cdr lst) 
                              (append pre (list (car lst))))
                       )))]
      (split lst '()))))

(define is_quantifier?
  (lambda (x)
    (or (equal? x '∀)
        (equal? x '∃))))

(define build_arb
  (lambda (exp op)
    (if (and (equal? op 'not)
             (equal? 'not (car exp)))
        (make-Node 'not (cdr exp) 'nil)
        (if (is_quantifier? op)
            (make-Node op (cadr exp) (cddr exp))
            (let* [(subarbs (split_list op exp))
                   (left_arb (if (= (length (car subarbs)) 1)
                                 (caar subarbs)
                                 (car subarbs)))
                   (right_arb (if (= (length (cadr subarbs)) 1)
                                  (caadr subarbs)
                                  (cadr subarbs)))
                   ]
              (make-Node op left_arb right_arb))))))

(define operators '(<-> -> or and not ∃ ∀))

(define operators_prior
  (map (lambda (x y) (list x y)) 
       operators
       (build-list (length operators) values)))

(define more_priority
  (lambda (op1 op2)
    (let [[find_op1 (assoc op1 operators_prior)]
          [find_op2 (assoc op2 operators_prior)]]
      (if (or (not find_op1) 
              (not find_op2))
          #f
          (> (second find_op1)
             (second find_op2))))))

(define strip_braces
  (lambda (tree)
    (if (equal? 'nil tree)
        '()
        (if (and (not (member (Node-info tree) operators))
                 (not (is_pred_node tree))
                 (not (is_func_node tree)))
            (list (Node-info tree))
            (let* [[get_subarb 
                    (lambda (subtree)
                      (if (equal? 'nil subtree)
                          '()
                          (if (more_priority (Node-info tree) (Node-info subtree))
                              (list (strip_braces subtree))
                              (strip_braces subtree))))]
                   [left_arb (get_subarb (Node-left tree))]
                   [right_arb (get_subarb (Node-right tree))]
                   [only_left (lambda () (append (list (Node-info tree)) left_arb))]
                   [root_first (lambda () (append (list (Node-info tree)) left_arb right_arb))]
                   [infix_form (lambda () (append left_arb (list (Node-info tree)) right_arb))]
                   ]
              (case (Node-info tree)
                ['not (only_left)]
                ['∃ (root_first)]
                ['∀ (root_first)]
                [else 
                 (cond [(is_pred_node tree) (append (list (Node-info tree))
                                                           (list left_arb))]
                       [(is_func_node tree) (append (list (car (Node-info tree)))
                                                           (list (cdr (Node-info tree))))]
                       [else (infix_form)])
                 ]
                ))))))

(define find_root 
  (lambda (ops)
    (lambda (exp)
      (if (null? ops)
          (if (isFunc? exp)
              (make-Node (cons (car exp) (cadr exp)) 'nil 'nil)
              (if (isPred? exp)
                  (make-Node (car exp) ((find_root operators) (cadr exp)) 'nil)
                  (if (and (list? exp) (not (null? exp)))
                      ((find_root operators) exp)
                      (make-Node exp 'nil 'nil))))
          (if (equal? exp 'nil)
              'nil
              (if (not (list? exp))
                  (make-Node exp 'nil 'nil)
                  (if (= (length exp) 1)
                      ((find_root operators) (car exp))
                      (if (member (car ops) exp)
                          (let [(arb (build_arb exp (car ops)))]
                            (make-Node (Node-info arb) 
                                       ((find_root operators) (Node-left arb))
                                       ((find_root operators) (Node-right arb))))
                          ((find_root (cdr ops)) exp)))))))))

(define build_trees
  (lambda (formulas)
    (map (find_root operators) formulas)))

(define is-leaf?
  (lambda (tree)
    (and (not (equal? 'nil tree)) 
         (equal? 'nil (Node-left tree))
         (equal? 'nil (Node-right tree)))))

(define dimension_arb
  (lambda (tree)
    (if (equal? tree 'nil)
          0
          (+ 1 
             (dimension_arb (Node-left tree))
             (dimension_arb (Node-right tree))))))

(define equal_arbs?
  (lambda (arb1 arb2)
    (if (and (equal? 'nil arb1) (equal? 'nil arb2))
        #t
        (if (or (equal? arb1 'nil) (equal? arb2 'nil))
            #f
            (if (and (is-leaf? arb1) (is-leaf? arb2))
                (if (equal? (Node-info arb1) (Node-info arb2))
                    #t
                    #f)
                (or (and (equal_arbs? (Node-left arb1)
                                      (Node-left arb2))
                         (equal_arbs? (Node-right arb1)
                                      (Node-right arb2)))
                    (and (equal_arbs? (Node-left arb1)
                                      (Node-right arb2))
                         (equal_arbs? (Node-right arb1)
                                      (Node-left arb2))))
                )))))

(define remove_not 
  (lambda (tree)
    (if (equal? tree 'nil) 'nil
        (if (is-leaf? tree) tree
            (if (and (equal? (Node-info tree) 'not)
                     (equal? (Node-info (Node-left tree)) 'not))
                (remove_not (Node-left (Node-left tree)))
                (make-Node (Node-info tree)
                           (remove_not (Node-left tree))
                           (remove_not (Node-right tree)))
                )))))

(define formulas (read_expressions "input.txt"))
;formulas
(define eval_trees (build_trees formulas))
;eval_trees
(strip_braces (last eval_trees))