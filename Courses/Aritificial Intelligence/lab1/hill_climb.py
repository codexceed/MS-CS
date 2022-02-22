#!/usr/bin/env python

import json
import random
from argparse import ArgumentParser
from abc import abstractmethod
from copy import copy
from typing import Optional, List, Tuple

# Constants
VERBOSE, NQUEENS, SIDEWAYS, RESTARTS = False, None, None, None


class FailedSearch(Exception):
    pass


class HillClimbingSolver:
    def __init__(self, start):
        self.start_state = start

    @abstractmethod
    def set_random_start(self) -> None:
        """Randomly generate a new start state"""
        pass

    def goal(self, state) -> bool:
        return self.error(state) == 0

    @abstractmethod
    def error(self, state) -> int:
        pass

    @abstractmethod
    def next(self, state) -> list:
        pass

    def solve(self) -> Tuple[List[str], int]:
        """Execute hill climbing algorithm to optimize towards the goal state from the start state.

        Returns:
            Goal state.
        Raises:
            FailedSearch: If the goal state cannot be reached.
        """
        state, error = self.start_state, self.error(self.start_state)
        while not self.goal(state):
            neighbours = sorted(self.next(state), key=lambda x: x[1])

            if not neighbours or neighbours[0][1] >= error:
                global RESTARTS
                if RESTARTS:
                    RESTARTS -= 1
                    self.set_random_start()
                    return self.solve()

                raise FailedSearch("This search cannot reach goal.")

            state, error = neighbours[0]
            print(f"choose [{' '.join(map(str, state))}] = {error}")

        return state, error


class KnapSackSolver(HillClimbingSolver):
    def __init__(
        self, goods: List[dict], start: List[str], max_weight: int, target: int
    ):
        self.goods = {item["name"]: item for item in goods}
        self.max_weight = max_weight
        self.target = target
        super(KnapSackSolver, self).__init__(start)

    def set_random_start(self) -> None:
        self.start_state = random.sample(self.goods, random.randint(1, len(self.goods)))

    def error(self, state: List[str]) -> int:
        if not state:
            return self.target

        return max(
            sum(self.goods[item]["W"] for item in state) - self.max_weight, 0
        ) + max(self.target - sum(self.goods[item]["V"] for item in state), 0)

    def next(self, state: List[str]) -> List[Tuple[List[str], int]]:
        """Get all the neighbouring states.

        Neighbouring states can be reached by either removing, adding or swapping out an item in the sack.

        Args:
            state(List[str]): Current state

        Returns:
            List of reachable (neighbouring) states alongside their corresponding error values.
        """
        neighbours = []

        for item in sorted(self.goods):
            if item in state:
                continue
            new_state = sorted(state + [item])
            neighbours.append((new_state, self.error(new_state)))

        for i, curr_item in enumerate(state):
            t_state = state[:i] + state[i + 1 :]
            neighbours.append((t_state, self.error(t_state)))
            for item in self.goods:
                if item == curr_item or item in t_state:
                    continue
                new_state = sorted(t_state + [item])
                neighbours.append((new_state, self.error(new_state)))

        if VERBOSE:
            for i in neighbours:
                print(f"[{' '.join(i[0])}] = {i[1]}")

        return neighbours


class NQueensSolver(HillClimbingSolver):
    def set_random_start(self) -> None:
        random.shuffle(self.start_state)

    def error(self, state: List[int]) -> int:
        """Get error for current state based on number of diagonal interactions.

        Args:
            state: Values indicating arrangement of queens

        Returns:
            Error value.
        """
        error = 0
        for col, row in enumerate(state):
            for col_o, row_o in enumerate(state[col + 1 :], col + 1):
                if abs(col - col_o) == abs(row - row_o):  # Diagonal condition
                    error += 1

        return error

    def next(self, state: List[int]) -> List[Tuple[List[int], int]]:
        """Get all the neighbouring states.

        Neighbouring states can be identified by swapping row assignments.

        Args:
            state: Current state

        Returns:
            List of reachable (neighbouring) states alongside their corresponding error values.
        """
        neighbours = []
        for col, row in enumerate(state):
            for col_o, row_o in enumerate(state[col + 1 :], col + 1):
                new_state = copy(state)
                new_state[col], new_state[col_o] = row_o, row
                error = self.error(new_state)
                neighbours.append((new_state, error))
                if VERBOSE:
                    print(f"[{' '.join(map(str, new_state))}] = {error}")

        return neighbours


def parse_cli_args() -> Optional[str]:
    """
    Parse CLI input, set globals.

    Returns:
        Knapsack file for hill climbing
    """
    parser = ArgumentParser(
        description="Solve given problem using hill climbing algorithm."
    )
    parser.add_argument(
        "-v",
        "-verbose",
        dest="verbose",
        help="Enable verbose output",
        action="store_true",
        default=False,
    )
    parser.add_argument(
        "-N",
        dest="nqueens",
        type=int,
        help="Number of queens to position in N-queens",
        default=None,
    )
    parser.add_argument("-sideways", help="Number of allowed sideways steps", type=int)
    parser.add_argument(
        "-restarts",
        help="Number of allowed random restarts",
        action="store_true",
        default=False,
    )
    parser.add_argument(
        "knapsack_file",
        help="Path to knapsack input file",
        type=str,
        nargs="?",
        default=None,
    )
    args = parser.parse_args()

    if args.nqueens and args.knapsack_file:
        raise Exception("Input for multiple problems provided.")

    global VERBOSE, NQUEENS, SIDEWAYS, RESTARTS
    VERBOSE, NQUEENS, SIDEWAYS, RESTARTS = (
        args.verbose,
        args.nqueens,
        args.sideways,
        args.restarts,
    )

    return args.knapsack_file


if __name__ == "__main__":
    file = parse_cli_args()
    solver = None

    if file:
        inp = json.load(open(file, "r"))
        T, M, start_state, items = (
            int(inp["T"]),
            int(inp["M"]),
            inp["Start"],
            inp["Items"],
        )
        solver = KnapSackSolver(goods=items, start=start_state, max_weight=M, target=T)

    elif NQUEENS:
        start_state = list(range(NQUEENS))
        solver = NQueensSolver(start_state)

    if solver is None:
        raise Exception("Invalid input. Target problem solver could not be identified.")
    print(f"Start [{' '.join(map(str, start_state))}] = {solver.error(start_state)}")
    goal_state, goal_error = solver.solve()
    print(f"Goal [{' '.join(map(str, goal_state))}] = {goal_error}")
