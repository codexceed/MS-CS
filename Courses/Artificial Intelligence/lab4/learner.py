#!/usr/bin/env python3
from argparse import ArgumentParser
from typing import Tuple, List

from classifiers import KNNClassifier, NaiveBayesClassifier


def print_metrics(test: List[str], predicted: List[str], labels: List[str]) -> None:
    """Print Precision and Recall metrics for each label in test set

    Args:
        test: Test labels
        predicted: Predicted labels
        labels: Trained labels

    Returns:
        None
    """
    comparison_zip = list(zip(test, predicted))
    for label in labels:
        actual_positives = list(filter(lambda x: x[0] == label, comparison_zip))
        predicted_positives = list(filter(lambda x: x[1] == label, comparison_zip))

        true_positives = sum([a == b for a, b in actual_positives])
        print(
            f"Label={label} Precision={true_positives}/{len(predicted_positives)} Recall={true_positives}/{len(actual_positives)}"
        )


def parse_csv(file: str) -> Tuple[List[Tuple[int]], List[str]]:
    """Parse data points from csv file

    Args:
        file: Path to csv file

    Returns:
        Extracted features and labels
    """
    points, labels = [], []
    with open(file, "r") as f:
        for line in f:
            tokens = line.strip("\n").split(",")
            if len(tokens) < 2:
                break

            points.append(tuple(map(int, tokens[:-1])))
            labels.append(tokens[-1])

    return points, labels


def parse_cli_args() -> Tuple:
    """Parse CLI input, set globals.

    Returns:
        Argument values from CLI
    """
    parser = ArgumentParser(description="Generic Classification-Evaluation Program")
    parser.add_argument("-train", help="Training file path", type=str, required=True)
    parser.add_argument("-test", help="Test file path", type=str, required=True)
    parser.add_argument(
        "-K", help="Number of neighbours for KNN classification", type=int, default=0
    )
    parser.add_argument(
        "-C", help="Laplacian correction for Naive Bayes", type=int, default=0
    )
    parser.add_argument(
        "-verbose",
        dest="verbose",
        help="Enable verbose output",
        action="store_true",
        default=False,
    )
    args = parser.parse_args()

    if args.K != 0 and args.C != 0:
        raise Exception(
            "Invalid input args. Cannot have non-zero values for both 'K' and 'C'"
        )

    return (
        args.train,
        args.test,
        args.K if args.K else args.C,
        KNNClassifier if args.K else NaiveBayesClassifier,
        args.verbose,
    )


if __name__ == "__main__":
    train_file, test_file, clf_param, classifier_cls, verbose = parse_cli_args()

    # Parse training data
    training_pts, labels = parse_csv(train_file)

    # Train/Setup Classifier
    classifier = classifier_cls(training_pts, labels, verbose)

    # Test classifier
    test_points, test_labels = parse_csv(test_file)
    predictions = []
    for i, point in enumerate(test_points):
        # Predict label
        predictions.append(classifier.classify(point, clf_param))
        if verbose:
            if isinstance(classifier, KNNClassifier):
                print(f"want={test_labels[i]} got={predictions[i]}")

            elif isinstance(classifier, NaiveBayesClassifier):
                if test_labels[i] == predictions[i]:
                    print(f'match: "{test_labels[i]}"')
                else:
                    print(f'fail: got "{predictions[i]}" != want "{test_labels[i]}"')

    print_metrics(test_labels, predictions, classifier.unique_labels)
