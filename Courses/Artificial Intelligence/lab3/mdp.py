import re
from argparse import ArgumentParser
from enum import Enum
from typing import Tuple, Dict, Union, List


class ParseRegex(Enum):
    Name = r"^[A-z]+"
    Value = r"\-?\d+"
    Transitions = r"\[[(\w+,)\s]*\w\s*\]"
    Probabilities = r"(\d*\.\d+\s+)*\d*\.\d+"


# class MDP:
#     def __init__(self, input):


def input_parser(
    input_text: str,
) -> Tuple[Dict[str, int], Dict[str, Dict[str, Union[List[str], List[float]]]]]:
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

            rewards[node] = value

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
            transitions[node]["nodes"] = transition_nodes

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

            if sum(probabilities) != 1:
                raise Exception(f"Invalid Probabilities for node {node}")

            # Check for zero reward transitions
            if rewards.get(node, None) is None:
                rewards[node] = 0

            if transitions.get(node, None) is None:
                transitions[node] = {}

            if len(probabilities) == 1:
                transitions[node]["decision"] = True
            else:
                transitions[node]["decision"] = False

            transitions[node]["probabilities"] = probabilities

    # Sanitize elements
    for node, transition in transitions.items():
        try:
            trans_nodes, probs = transition["nodes"], transition["probabilities"]
        except Exception:
            trans_nodes = transition["nodes"]
            probs = [1]
            transition["probabilities"] = probs
            transition["decision"] = True
            transitions[node] = transition

        if len(trans_nodes) != len(probs) and not transition["decision"]:
            raise Exception(f"Invalid Transition entries for node {node}")

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

    print(rewards, transitions)
    # solver = MDP()
