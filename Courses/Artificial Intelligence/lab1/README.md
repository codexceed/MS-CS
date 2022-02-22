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
 python3 hill_climb.py -verbose -sideways 3 test_cases/knapsack1.txt
 Start [A E] = 6
[A B E] = 3
[A C E] = 2
[A D E] = 2
.
.
.
[B E] = 8
[A B E] = 3
[B D E] = 2
[B C] = 5
[A B C] = 5
[B C D] = 0
choose [B C D] = 0
Goal [B C D] = 0
```

```shell
python3 hill_climb.py -verbose -sideways 5 -restarts 5 -N 16
Start [0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15] = 120
[1 0 2 3 4 5 6 7 8 9 10 11 12 13 14 15] = 92
[2 1 0 3 4 5 6 7 8 9 10 11 12 13 14 15] = 94
.
.
.
[7 11 15 0 9 4 12 3 6 13 10 1 8 2 5 14] = 4
[7 11 15 0 9 4 12 3 6 13 10 1 14 5 2 8] = 2
[7 11 15 0 9 4 12 3 6 13 10 1 14 8 5 2] = 2
[7 11 15 0 9 4 12 3 6 13 10 1 14 2 8 5] = 4
Jump sideways to [7 10 15 0 9 4 12 3 6 13 11 1 14 2 5 8] = 0
choose [7 10 15 0 9 4 12 3 6 13 11 1 14 2 5 8] = 0
Goal [7 10 15 0 9 4 12 3 6 13 11 1 14 2 5 8] = 0
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