# AI | Lab 3 | Markov Decision Process Solver
This is a python implementation of the **MDP** algorithm.
## Requirements
- Python 3

## Usage    
```shell
python3 mdp.py <path-to-input-file>
```
### Example
```shell
python3 mdp.py test_cases\restaurant\input
BusyChi -> GoInd
BusyInd -> Eat
Office -> Ind

BusyChi=59.000  BusyInd=52.500  Chi=44.500  Eat=52.500  Fast=30.000  GoChi=49.500  GoInd=59.000  Ind=49.000  Office=49.000  Quiet=10.000  Slow=60.000
```

## Help and other options
You can access help on additional options as follows
```shell
$ python mdp.py -h
usage: mdp.py [-h] [-df DF] [-min] [-tol TOL] [-iter ITER] [input-file]

Generic Markov Decision Process Solver

positional arguments:
  input-file  Path to input file

optional arguments:
  -h, --help  show this help message and exit
  -df DF      Discount Factor
  -min        Minimize values
  -tol TOL    Tolerance
  -iter ITER  Discount Factor
```