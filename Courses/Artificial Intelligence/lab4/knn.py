import math
from typing import List, Tuple


def euclidean_distance(x1: Tuple[int], x2: Tuple[int]) -> float:
    """Return euclidean between given pair of n-dimensional points"""
    if len(x1) != len(x2):
        raise Exception("Dimension mismatch.")

    return math.sqrt(sum([(a - b) ** 2 for a, b in zip(x1, x2)]))


class KNNClassifier:
    def __init__(self, points: List[Tuple[int]], labels: List[str]):
        if len(points) != len(labels):
            raise Exception("Invalid input. Number of points and labels don't match.")

        self.points = points
        self.labels = labels

    @property
    def unique_labels(self):
        return sorted(list(set(self.labels)))

    def classify(self, point: Tuple[int], n_voters: int) -> str:
        """Classify given point to the appropriate label.

        Args:
            point: n-dimensional point matching the classifier training dimensions
            n_voters: Number of voters to consider for KNN classification

        Returns:
            Label
        """
        # Compute and sort distances to get K nearest neighbours
        distances = sorted(
            [
                (i, euclidean_distance(clf_point, point))
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
