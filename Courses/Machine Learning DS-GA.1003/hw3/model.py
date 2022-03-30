import random
import numpy as np
import statistics as st
from collections import Counter
from datetime import datetime
from typing import List, Dict
from utils_svm_reviews import *


def classification_error(x: List[dict], w: Dict[str, float], y: List[int]) -> float:
    """Calculate 0-1 loss for given data and weights.

    Args:
        x: Feature vector
        w: weights
        y: Output labels

    Returns:
        0-1 loss error
    """
    correct = 0.0
    for i, x_vec in enumerate(x):
        pred = dotProduct(w, x_vec)
        if np.sign(pred) == y[i]:
            correct += 1

    return correct / len(x)


def train_pegasos(x: List[dict], y: List[int], lam: float, epochs: int = 2) -> dict:
    """Train weights corresponding to training data represented as sparse matrix.

    Args:
        x: Training features
        y: Training output class
        lam: Regularization factor
        epochs: Number of epochs to run

    Returns:
        Sparse matrix of trained weights.
    """
    seeds, w = list(range(epochs)), {}
    print("Naive Pegasos SGD")
    for j in range(epochs):
        print(f"Epoch: {j}")
        random.seed(seeds[j])
        random.sample(x, k=len(x))
        for i in range(len(x)):
            eta = 1.0 / ((i + 2) * lam)
            margin = y[i] * dotProduct(w, x[i])
            scale = 1.0 - eta * lam

            for (
                k,
                v,
            ) in w.items():
                w[k] = v * scale

            if margin < 1.0:
                increment(w, eta * y[i], x[i])

    return w


def train_pegasos_opt(x: List[dict], y: List[int], lam: float, epochs: int = 2) -> dict:
    """Train weights corresponding to training data represented as sparse matrix.

    Args:
        x: Training features
        y: Training output class
        lam: Regularization factor
        epochs: Number of epochs to run

    Returns:
        Sparse matrix of trained weights.
    """
    seeds, s, w = list(range(epochs)), 1.0, {}
    print("Optimized Pegasos SGD")
    for j in range(epochs):
        print(f"Epoch: {j}")
        random.seed(seeds[j])
        random.sample(x, k=len(x))
        for i in range(len(x)):
            eta = 1.0 / ((i + 2) * lam)

            margin = y[i] * dotProduct(w, x[i])
            s *= 1.0 - eta * lam

            if margin < 1.0:
                increment(w, (eta / s) * y[i], x[i])

    return {k: s * v for k, v in w.items()}


if __name__ == "__main__":
    # Utils
    def get_bag_of_words(words):
        return Counter(words)

    # Load Data
    data = load_and_shuffle_data()
    data = [(get_bag_of_words(line[:-1]), line[-1]) for line in data]

    X_train, y_train = [rev[0] for rev in data[:1500]], [rev[1] for rev in data[:1500]]
    X_test, y_test = [rev[0] for rev in data[1500:]], [rev[1] for rev in data[1500:]]

    n_epoch, lam_coeff = 4, 0.01

    start = datetime.now()
    w1 = train_pegasos(X_train, y_train, lam_coeff, epochs=n_epoch)
    end = datetime.now()
    print(f"Naive Pegasos Runtime: {(end - start).microseconds}\n")

    start = datetime.now()
    w2 = train_pegasos_opt(X_train, y_train, lam_coeff, epochs=n_epoch)
    end = datetime.now()
    print(f"Optimized Pegasos Runtime: {(end - start).microseconds}")

    diff = {k: v - w2.get(k, 0) for k, v in w1.items()}
    print(st.mean(diff.values()))

    ax.table(
        cellText=w_pairs,
        rowLabels=sample_keys,
        colLabels=["Naive", "Optimized"],
        rowColours=["palegreen"] * 10,
        colColours=["palegreen"] * 10,
        cellLoc="center",
        loc="center",
    )
