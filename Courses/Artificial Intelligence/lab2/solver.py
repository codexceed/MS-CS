from argparse import ArgumentParser
from typing import Tuple

from dpll import DPLLSolver
from bnf2cnf import convert


def parse_cli_args() -> Tuple:
    """Parse CLI input, set globals.

    Returns:
        Argument values from CLI
    """
    parser = ArgumentParser(
        description="Solve BNF and CNF using DPLL"
    )
    parser.add_argument(
        "-verbose",
        dest="verbose",
        help="Enable verbose output",
        action="store_true",
        default=False,
    )
    parser.add_argument(
        "-mode",
        help="Operation mode",
        required=True,
        nargs=2,
        metavar=("mode", "input-file")
    )
    args = parser.parse_args()

    mode, input_file = args.mode

    return mode, args.verbose, input_file


if __name__ == "__main__":
    mode, verbose, input_file = parse_cli_args()

    with open(input_file, "r") as input_f:
        lines = [line for line in input_f.read().split("\n") if line]

    if mode == "cnf":
        print("\n".join(convert(lines, verbose)))
    elif mode == "dpll":
        solver = DPLLSolver(lines, verbose)
        assignments = solver.solve()
        if assignments:
            print("\n".join([f"{k} = {str(v).lower()}" for k,v in sorted(assignments.items(), key=lambda x: x[0])]))
    elif mode == "solver":
        solver = DPLLSolver(convert(lines, verbose), verbose)
        assignments = solver.solve()
        if assignments:
            print("\n".join([f"{k} = {str(v).lower()}" for k,v in sorted(assignments.items(), key=lambda x: x[0])]))
    else:
        raise Exception("Invalid Mode.")
