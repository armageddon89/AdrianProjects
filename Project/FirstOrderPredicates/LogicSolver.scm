#lang racket
(require "helpers.scm")

(define formulas (read_expressions "input.txt"))
formulas

(define eval_trees (map (find_root operators) formulas))

(define remove_implications
  (lambda (tree)
    (if (equal? tree 'nil) 
        'nil
        (case (Node-info tree)
          ['<-> (remove_implications (make-Node 'and
                                                (make-Node '->
                                                           (remove_implications (Node-left tree))
                                                           (remove_implications (Node-right tree)))
                                                (make-Node '->
                                                           (remove_implications (Node-right tree))
                                                           (remove_implications (Node-left tree)))))]
          ['-> (remove_implications (make-Node 'or 
                                               (make-Node 'not 
                                                          (remove_implications (Node-left tree))
                                                          'nil)
                                               (remove_implications (Node-right tree))))]
          [else (make-Node (Node-info tree) 
                           (remove_implications (Node-left tree))
                           (remove_implications (Node-right tree)))]
          ))))

(define ex1
  (map remove_not (map remove_implications eval_trees)))

(define display1
  (map strip_braces ex1))

(display "_______________ Display 1 ________________\n\n")
display1

(define push_negations
  (lambda (tree)
    (if (equal? tree 'nil)
        'nil
        (let [[no_transform (lambda () 
                              (make-Node (Node-info tree)
                                         (push_negations (Node-left tree))
                                         (push_negations (Node-right tree))))]]
          (if (equal? (Node-info tree) 'not)
              (case (Node-info (Node-left tree))
                ['not (push_negations (Node-left (Node-left tree)))]
                ['and (make-Node 'or
                                 (push_negations (make-Node 'not
                                                            (Node-left (Node-left tree))
                                                            'nil))
                                 (push_negations (make-Node 'not
                                                            (Node-right (Node-left tree))
                                                            'nil)))]
                
                ['or (make-Node 'and
                                (push_negations (make-Node 'not
                                                           (Node-left (Node-left tree))
                                                           'nil))
                                (push_negations (make-Node 'not
                                                           (Node-right (Node-left tree))
                                                           'nil)))]
                ['∃ (make-Node '∀
                               (Node-left (Node-left tree))
                               (push_negations (make-Node 'not
                                                          (Node-right (Node-left tree))
                                                          'nil)))]
                ['∀ (make-Node '∃
                               (Node-left (Node-left tree))
                               (push_negations (make-Node 'not
                                                          (Node-right (Node-left tree))
                                                          'nil)))]
                [else (no_transform)])
              (no_transform)
            )))))

(define replace_var
  (lambda (var with tree)
    (if (equal? 'nil tree)
        'nil
        (if (is_func_node tree)
            (make-Node (map (lambda (x)
                              (if (equal? x var) with x))
                            (Node-info tree))
                       'nil 'nil)
            (let [[root (if (equal? var (Node-info tree)) 
                            with
                            (Node-info tree))]]
              (make-Node root
                         (replace_var var with (Node-left tree))
                         (replace_var var with (Node-right tree))))))))
                

(define rename_variables
  (lambda (tree)
    (if (equal? 'nil tree)
        'nil
        (if (is_quantifier? (Node-info tree))
            (let [[placeholder (get_next_unique)]]
              (make-Node (Node-info tree)
                         (make-Node placeholder 'nil 'nil)
                         (rename_variables (replace_var (Node-info (Node-left tree))
                                                        placeholder
                                                        (Node-right tree)))
                         ))
            (make-Node (Node-info tree)
                       (rename_variables (Node-left tree))
                       (rename_variables (Node-right tree)))))))
  
(define display2 (map push_negations display1))
(display "\n\n____________ Display  2 ___________\n\n")
(map strip_braces display2)

(define display3 (map rename_variables display2))
(map strip_braces display3)