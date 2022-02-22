# AI | Lab 1 | HIll Climbing Solver
This is a python implementation of the **Hill-Climbing** algorithm based solver for the **Knapsack** and **N-Queens** problems.
## Requirements
- Python 3
## Usage
### Basic
- **For Knapsack**
```shell
python3 hill_climb.py <path-to-input-file>
```
- **For N-Queens**
```shell
python3 hill_climb.py -N <number-of-queens>
```
### Advanced
```shell
 python hill_climb.py -verbose -sideways 3 test_cases/knapsack1.txt
 Start [A E] = 6
[A B E] = 3
[A C E] = 2
[A D E] = 2
.
.
.
choose [B C E] = 1
[A B C E] = 6
[B C D E] = 1
[C E] = 9
[A C E] = 2
[C D E] = 3
[B E] = 8
[A B E] = 3
[B D E] = 2
[B C] = 5
[A B C] = 5
[B C D] = 0
choose [B C D] = 0
Goal [B C D] = 0
```
## Help and other options
You can access help on additional options as follows
```shell
python3 hill_climb.py -h
usage: hill_climb.py [-h] [-verbose] [-N NQUEENS] [-sideways SIDEWAYS] [-restarts RESTARTS] [knapsack-file]

Solve given problem using hill climbing algorithm.

positional arguments:
  knapsack-file       Path to knapsack input file

optional arguments:
  -h, --help          show this help message and exit
  -verbose            Enable verbose output
  -N NQUEENS          Number of queens to position in N-queens
  -sideways SIDEWAYS  Number of allowed sideways steps
  -restarts RESTARTS  Number of allowed random restarts
```