import numpy as np
from matplotlib import pyplot as plt


def exp_cdf(a: np.ndarray, lam: float):
    return np.where(a >= 0, 1 - np.exp(-1*lam*a), 0)

x = np.load("samples.npy")
y = exp_cdf(x, lam=1)

plt.hist2d(x, y, bins=150)
plt.xlabel("x")
plt.ylabel("F(x)")
plt.show()
