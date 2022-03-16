# AI | Lab 2 | BNF to CNF DPLL Solver
This is a python implementation of the **DPLL** algorithm with built-in BNF to CNF conversion
## Requirements
- Python 3

## Usage    
```shell
python3 solver.py -mode <mode> <path-to-input-file>
```
### Basic
```shell
python3 solver.py -mode solver test_cases/bnf2cnf/case1/input
A = true
B = true
C = true
```
### Verbose Mode
```shell
python3 solver.py -mode solver test_cases/bnf2cnf/case1/input -verbose
Converting BNF to CNF
A <=> B <=> C
((((A  =>  B) & ( B => A ))  =>  C) & ( C => ((A  =>  B) & ( B => A )) ))
((!(((!(A) | B) & (!(B) | A))) | C) & (!(C) | ((!(A) | B) & (!(B) | A))))
((((A & ! B) | (B & ! A)) | C) & ((! C) | (((! A) | B) & ((! B) | A))))
A  B  C & !B  B  C & A  !A  C & !B  !A  C & !C  !A  B & !C  !B  A
A | !B
(A | !B)
(A | !B)
(A | ! B)
A  !B

Final CNF statements
A  B  C
!B  B  C
A  !A  C
!B  !A  C
!C  !A  B
!C  !B  A
A  !B 


Solving CNF using DPLL
A  B  C
!B  B  C
A  !A  C
!B  !A  C
!C  !A  B
!C  !B  A
A  !B
A  B  C
!B  B  C
A  !A  C
!B  !A  C
!C  !A  B
!C  !B  A
A  !B
hard case: guess A=true
!B  B  C
A C
!B C
!C B
hard case: guess B=true
B C
A C
C
easy case: unit literal C=true
A = true
B = true
C = true
```

## Help and other options
You can access help on additional options as follows
```shell
python3 solver.py -h
usage: solver.py [-h] [-verbose] -mode mode input-file

Solve BNF and CNF using DPLL

optional arguments:
  -h, --help            show this help message and exit
  -verbose              Enable verbose output
  -mode mode input-file
                        Operation mode
```