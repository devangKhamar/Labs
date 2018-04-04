# Labs

EDA Lab Work

Reduced Order Binary Decision Diagrams, Lab-1:

ROBDDs are a data structure that can be used to represent combinational circuits. It models a combinational circuit as a graph. 
ROBDDs allow easy evaluation of the combinational circuit and also allow for various tests to be conducted on the circuit such
as testing for satisfiability, checking for number of inpts that satisfy a given circuit and performing these tests for a 
sub-section of the circuit. 

This project implements an ROBDD constructor. The project implements several oprations that can be performed on ROBDDs sch as 
testing for satisfiability, computing number of satisfiable inputs and also restricting the ROBDD for deriving ROBDDs of 
subsections of the circuit.

The code takes as input a boolean expression of the following format:

((!x1 OR x3) IMPL (x2 EQUIV  (x4 AND x7)) should be passed as (IMPL (OR x3 (NOT x1)) (EQUIV x2 (AND x4 x7))).

The project implements a simple recursive descent parser to construct a syntax tree for the boolean expression and then uses 
the syntax tree to construct the ROBDD.

Traveling Tournament Simulated Annealing Alogrithm - made fair, Lab2:
