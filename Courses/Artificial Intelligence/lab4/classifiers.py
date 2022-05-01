from abc import ABC, abstractmethod
from functools import reduce

import math
from typing import List, Tuple


def euclidean_distance(x1: Tuple[int], x2: Tuple[int]) -> float:
    """Return euclidean between given pair of n-dimensional points"""
    if len(x1) != len(x2):
        raise Exception("Dimension mismatch.")

    return math.sqrt(sum([(a - b) ** 2 for a, b in zip(x1, x2)]))


class ClassifierBase(ABC):
    def __init__(
        self, points: List[Tuple[int]], labels: List[str], verbose: bool = False
    ):
        if len(points) != len(labels):
            raise Exception("Invalid input. Number of points and labels don't match.")

        self.points = points
        self.labels = labels
        self.verbose = verbose

    @abstractmethod
    def classify(self, features: Tuple[int], *args) -> str:
        pass


class NaiveBayesClassifier(ClassifierBase):
    def __init__(
        self, points: List[Tuple[int]], labels: List[str], verbose: bool = False
    ):
        super().__init__(points, labels, verbose)
        self.data_zip = list(zip(self.points, self.labels))
        self.unique_labels = sorted(list(set(self.labels)))
        self.unique_feature_vals = []
        for i in range(len(self.points[0])):
            self.unique_feature_vals.append(sorted(list(set([x[i] for x in self.points]))))

    def classify(self, features: Tuple[int], *args) -> str:
        """Classify given point to the appropriate label.

        Args:
            features: n-dimensional point matching the classifier training dimensions
            args: Additional parameter for number laplacian correct 'C'

        Returns:
            Label
        """
        if len(args) != 1 or not isinstance(args[0], int):
            raise Exception("Invalid arguments for Naive Bayes classifier.")

        laplacian = args[0]
        max_prob, max_label = 0, None
        for label in self.unique_labels:
            # Base label probability
            n_label = len(list(filter(lambda x: x == label, self.labels)))

            if self.verbose:
                print(
                    f"P(C={label}) = [{n_label + laplacian} / {len(self.labels) + len(self.unique_labels) * laplacian}]"
                )

            prob_label = (n_label + laplacian) / (
                len(self.labels) + len(self.unique_labels) * laplacian
            )

            # Feature probabilities
            feature_probs = []
            for i, feature in enumerate(features):
                # Count relevant data points
                n_joint = len(
                    list(
                        filter(
                            lambda x: x[1] == label and x[0][i] == feature,
                            self.data_zip,
                        )
                    )
                )
                n_base = len(list(filter(lambda x: x == label, self.labels)))
                unique_feature_vals = len(self.unique_feature_vals[i])

                if self.verbose:
                    print(
                        f"P(A{i}={feature} | C={feature}) = {n_joint + laplacian} / {n_base + unique_feature_vals * laplacian}"
                    )

                feature_probs.append(
                    (n_joint + laplacian)
                    / (n_base + unique_feature_vals * laplacian)
                )

            conditional_prob = prob_label * reduce(
                lambda p1, p2: p1 * p2, feature_probs
            )

            if self.verbose:
                print(f"NB(C={label}) = {conditional_prob}")

            # Pick the label with highest probability
            if conditional_prob > max_prob:
                max_prob = conditional_prob
                max_label = label

        return max_label


class KNNClassifier(ClassifierBase):
    def classify(self, features: Tuple[int], *args) -> str:
        """Classify given point to the appropriate label.

        Args:
            features: n-dimensional point matching the classifier training dimensions
            args: Additional parameter for number of voters 'K'

        Returns:
            Label
        """
        if len(args) != 1 or not isinstance(args[0], int):
            raise Exception("Invalid arguments for KNN classifier.")

        n_voters = args[0]

        # Compute and sort distances to get K nearest neighbours
        distances = sorted(
            [
                (i, euclidean_distance(clf_point, features))
                for i, clf_point in enumerate(self.points)
            ],
            key=lambda d: d[1],
        )[:n_voters]

        # Commence voting by neighbours
        label_votes = {}
        for i, distance in distances:
            votes = label_votes.get(self.labels[i], 0)

            if distance == 0:
                label_votes[self.labels[i]] = float("inf")
                continue

            votes += 1 / distance
            label_votes[self.labels[i]] = votes

        return max(label_votes, key=lambda label: label_votes[label])
