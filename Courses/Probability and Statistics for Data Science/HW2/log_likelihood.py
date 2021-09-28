import numpy as np
import matplotlib.pyplot as plt

# Initialize graph
fig = plt.figure()

# Set projection type to 3d
ax = plt.axes(projection="3d")

# Define your variables
theta, alpha = np.linspace(0.000000001, 1, num=100), np.linspace(
    0.000000001, 1, num=100
)
x, y = np.meshgrid(theta, alpha)
z = 4 * np.log(x) + 2 * np.log(y) + 4 * np.log(1 - x - y)

# Plot it
ax.contour3D(x, y, z, 700, cmap="binary")
ax.set_xlabel("\u03F4")
ax.set_ylabel("\u03B1")
ax.set_zlabel("log-likelihood")
plt.show()
