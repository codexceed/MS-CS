import re
import random
from argparse import ArgumentParser
from copy import deepcopy
from enum import Enum
from typing import Tuple, Dict, Union, List


class ParseRegex(Enum):
    Name = r"^[A-z0-9]+"
    Value = r"\-?\d+"
    Transitions = r"\[[(\w+,)\s]*\w\s*\]"
    Probabilities = r"(\d*\.\d+\s+)*\d*\.\d+"


class TransitionKeys(Enum):
    nodes = "nodes"
    probs = "probabilities"
    decision = "decision"


class MDP:
    def __init__(
        self,
        rewards: Dict[str, int],
        transitions: Dict[str, Dict[str, Union[List[str], List[float], bool]]],
    ):
        self.rewards = rewards
        self.transitions = transitions

    def _value_iteration(
        self,
        policy: Dict[str, str],
        discount: float,
        tolerance: float,
        max_iterations: int,
    ):
        """Perform value iterations on the current states.

        Args:
            policy: Transition policy
            max_iterations: Maximum number of iterations

        Returns:
            Final values
        """

        for _ in range(max_iterations):
            new_vals = deepcopy(self.values)
            for state, value in self.values.items():
                new_vals[state] = self.rewards[state]
                transitions = self.transitions.get(state, None)
                if transitions is None:
                    continue

                nodes = transitions[TransitionKeys.nodes.value]

                # Decision node
                if self.transitions[state][TransitionKeys.decision.value]:
                    prob = self.transitions[state][TransitionKeys.probs.value][0]
                    new_vals[state] += discount * (
                        prob * self.values[policy[state]]
                        + ((1 - prob) / len(nodes))
                        * sum([self.values[node] for node in nodes])
                    )

                # Chance node
                else:
                    probs = self.transitions[state][TransitionKeys.probs.value]
                    new_vals[state] += discount * (
                        sum(
                            [
                                prob * self.values[node]
                                for prob, node in zip(probs, nodes)
                            ]
                        )
                    )

            val_zip = [(val, self.values[key]) for key, val in new_vals.items()]
            if all([abs(a - b) <= tolerance for a, b in val_zip]):
                self.values.update(new_vals)
                break

            self.values.update(new_vals)

    def solve(
        self,
        discount: float,
        tolerance: float,
        max_iterations: int,
        min_flag: bool = False,
    ) -> Tuple[Dict[str, int], Dict[str, str]]:
        """Solve the given MDP states.

        Args:
            discount: Discount factor
            tolerance: Tolerance for cutting off the solver process
            max_iterations: Maximum number of value iterations
            min_flag: Set true to minimize value

        Returns:
            Solved state values and optimal policy
        """
        self.values = deepcopy(self.rewards)
        policy_states = [k for k, v in self.transitions.items() if v[TransitionKeys.decision.value]]
        policy = {key: random.choice(policy_states) for key in policy_states}
        update = True

        while update:
            update = False

            self._value_iteration(policy, discount, tolerance, max_iterations)

            # Check for policy updates
            for state in policy_states:
                critera = min if min_flag else max
                transitions = self.transitions.get(state, None)
                if transitions is None:
                    continue

                best_node = critera(
                    transitions[TransitionKeys.nodes.value],
                    key=lambda n: self.values[n],
                )
                if best_node != policy[state]:
                    update = True
                    policy[state] = best_node

        return self.values, policy


def input_parser(
    input_text: str,
) -> Tuple[Dict[str, int], Dict[str, Dict[str, Union[List[str], List[float], bool]]]]:
    """Parse input string for MDP.

    Args:
        input_text: MDP input

    Returns:
        MDP input elements.
    """
    rewards, transitions = {}, {}
    for line in [x.strip() for x in input_text.split("\n")]:
        if not line or line[0] == "#":
            continue

        node = re.search(ParseRegex.Name.value, line).group()
        # Parse values
        if re.match(ParseRegex.Name.value + r"\s*=\s*" + ParseRegex.Value.value, line):
            value = re.search(ParseRegex.Value.value, line).group()

            rewards[node] = int(value)

        # Parse transition nodes
        elif re.match(
            ParseRegex.Name.value + r"\s*:\s*" + ParseRegex.Transitions.value, line
        ):
            transition_nodes = [
                n.strip()
                for n in re.search(ParseRegex.Transitions.value, line)
                .group()
                .lstrip("[")
                .rstrip("]")
                .split(",")
            ]

            if rewards.get(node, None) is None:
                rewards[node] = 0

            # Check for zero reward transitions
            for trans in transition_nodes:
                if rewards.get(trans, None) is None:
                    rewards[trans] = 0

            if transitions.get(node, None) is None:
                transitions[node] = {}
            transitions[node][TransitionKeys.nodes.value] = transition_nodes

        # Parse transition probabilities
        elif re.match(
            ParseRegex.Name.value + r"\s*%\s*" + ParseRegex.Probabilities.value, line
        ):
            probabilities = list(
                map(
                    float,
                    re.search(ParseRegex.Probabilities.value, line).group().split(" "),
                )
            )

            if sum(probabilities) != 1 and len(probabilities) != 1:
                raise Exception(f"Invalid Probabilities for node {node}")

            # Check for zero reward transitions
            if rewards.get(node, None) is None:
                rewards[node] = 0

            if transitions.get(node, None) is None:
                transitions[node] = {}

            if len(probabilities) == 1:
                transitions[node][TransitionKeys.decision.value] = True
            else:
                transitions[node][TransitionKeys.decision.value] = False

            transitions[node][TransitionKeys.probs.value] = probabilities

    # Sanitize elements
    for node, transition in transitions.items():
        try:
            trans_nodes, probs = (
                transition[TransitionKeys.nodes.value],
                transition[TransitionKeys.probs.value],
            )
        except KeyError:
            trans_nodes = transition[TransitionKeys.nodes.value]
            probs = [1]
            transition[TransitionKeys.probs.value] = probs
            transition[TransitionKeys.decision.value] = True
            transitions[node] = transition

        if (
            len(trans_nodes) != len(probs)
            and not transition[TransitionKeys.decision.value]
        ):
            raise Exception(f"Invalid Transition entries for node {node}")

        if len(trans_nodes) == len(probs) == 1:
            transition[TransitionKeys.decision.value] = False

    return rewards, transitions


def parse_cli_args() -> Tuple:
    """Parse CLI input, set globals.

    Returns:
        Argument values from CLI
    """
    parser = ArgumentParser(description="Generic Markov Decision Process Solver")
    parser.add_argument(
        "-df",
        help="Discount Factor",
        type=float,
        default=1.0,
    )
    parser.add_argument(
        "-min",
        dest="minimize",
        help="Minimize values",
        action="store_true",
        default=False,
    )
    parser.add_argument(
        "-tol",
        help="Tolerance",
        type=float,
        default=0.01,
    )
    parser.add_argument(
        "-iter",
        help="Discount Factor",
        type=int,
        default=100,
    )
    parser.add_argument(
        "input_file",
        metavar="input-file",
        help="Path to input file",
        type=str,
        nargs="?",
        default=None,
    )
    args = parser.parse_args()

    return args.df, args.minimize, args.tol, args.iter, args.input_file


if __name__ == "__main__":
    discount, min_flag, tolerance, max_iterations, input_file = parse_cli_args()

    with open(input_file) as f:
        input_text = f.read()

    rewards, transitions = input_parser(input_text)

    solver = MDP(rewards=rewards, transitions=transitions)
    values, policy = solver.solve(discount, tolerance, max_iterations, min_flag)

    for src, dest in sorted(policy.items(), key=lambda x: x[0]):
        print(f"{src} -> {dest}")

    print(
        "\n",
        "  ".join(
            [
                f"{key}={val:.3f}"
                for key, val in sorted(values.items(), key=lambda x: x[0])
            ]
        ),
        sep="",
    )
