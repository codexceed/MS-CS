import numpy as np


def train_perceptron(X_train, y_train, epoch_lim=10, hinge_loss=False):
    """Train weights for given data via perceptron algorithm.

    Args:
        X_train: Feature matrix
        y_train: Class labels
        epoch_lim: Upper epoch limit to cut-off training
        hinge_loss: Specify whether to use hinge loss

    Returns:
        Trained weights
    """
    w = np.zeros(X_train.shape[1])
    epoch, convergence = 0, False
    threshold = 1 if hinge_loss else 0

    while not convergence and epoch < epoch_lim:
        convergence = True
        for i in range(len(X_train)):
            x = X_train[i]
            y = y_train[i]

            margin = y * np.dot(w, x)
            if margin <= threshold:
                convergence = False
                w += y * x

        epoch += 1

    return w


def predict_perceptron(x, weights):
    """Predict values using perceptron weights

    Args:
        x: Input features
        weights: Perceptron weights

    Return:
        Output vector
    """

    return np.sign(np.dot(x, weights))


if __name__ == '__main__':
    def generate_data(n):
        x = np.random.rand(n, 2)
        func = lambda x: 1 if x else -1
        y = np.vectorize(func)(x[:, 0] > x[:, 1])

        return x, y


    X_train, y_train = generate_data(10)

    w = train_perceptron(X_train, y_train)

    X_test, y_test = generate_data(5000)
    pred = predict_perceptron(X_test, w)

    print(f"Accuracy with perceptron loss: {sum(pred == y_test) / y_test.shape[0]}")