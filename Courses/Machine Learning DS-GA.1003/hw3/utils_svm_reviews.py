import os
import numpy as np
import random


def folder_list(path, label):
    """
    PARAMETER PATH IS THE PATH OF YOUR LOCAL FOLDER
    """
    filelist = os.listdir(path)
    review = []
    for infile in filelist:
        file = os.path.join(path, infile)
        r = read_data(file)
        r.append(label)
        review.append(r)
    return review


def read_data(file):
    """
    Read each file into a list of strings.
    Example:
    ["it's", 'a', 'curious', 'thing', "i've", 'found', 'that', 'when', 'willis', 'is', 'not', 'called', 'on',
    ...'to', 'carry', 'the', 'whole', 'movie', "he's", 'much', 'better', 'and', 'so', 'is', 'the', 'movie']
    """
    f = open(file)
    lines = f.read().split(" ")
    symbols = '${}()[].,:;+-*/&|<>=~" '
    words = map(
        lambda Element: Element.translate(str.maketrans("", "", symbols)).strip(), lines
    )
    words = filter(None, words)
    return list(words)


def load_and_shuffle_data():
    """
    pos_path is where you save positive review data.
    neg_path is where you save negative review data.
    """
    pos_path = "data/pos"
    neg_path = "data/neg"

    pos_review = folder_list(pos_path, 1)
    neg_review = folder_list(neg_path, -1)

    review = pos_review + neg_review
    random.shuffle(review)
    return review


# Taken from http://web.stanford.edu/class/cs221/ Assignment #2 Support Code
def dotProduct(d1, d2):
    """
    @param dict d1: a feature vector represented by a mapping from a feature (string) to a weight (float).
    @param dict d2: same as d1
    @return float: the dot product between d1 and d2
    """
    if len(d1) < len(d2):
        return dotProduct(d2, d1)
    else:
        return sum(d1.get(f, 0) * v for f, v in d2.items())


def increment(d1, scale, d2):
    """
    Implements d1 += scale * d2 for sparse vectors.
    @param dict d1: the feature vector which is mutated.
    @param float scale
    @param dict d2: a feature vector.

    NOTE: This function does not return anything, but rather
    increments d1 in place. We do this because it is much faster to
    change elements of d1 in place than to build a new dictionary and
    return it.
    """
    for f, v in d2.items():
        d1[f] = d1.get(f, 0) + v * scale
