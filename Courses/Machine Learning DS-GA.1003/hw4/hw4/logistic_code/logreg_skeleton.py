import numpy as np
from scipy.optimize import minimize


def f_objective(theta, X, y, l2_param=1):
    '''
    Args:
        theta: 1D numpy array of size num_features
        X: 2D numpy array of size (num_instances, num_features)
        y: 1D numpy array of size num_instances
        l2_param: regularization parameter

    Returns:
        objective: scalar value of objective function
    '''
    margin = y * (X @ theta)
    loss = (sum(log_inv_sigmoid(margin)) / margin.shape[0]) + l2_param * np.linalg.norm(theta)
    return loss


def fit_logistic_reg(X, y, objective_function, l2_param=1):
    '''
    Args:
        X: 2D numpy array of size (num_instances, num_features)
        y: 1D numpy array of size num_instances
        objective_function: function returning the value of the objective
        l2_param: regularization parameter
        
    Returns:
        optimal_theta: 1D numpy array of size num_features
    '''
    return minimize(objective_function, np.zeros(X.shape[1]), args=(X, y, l2_param)).x


def log_inv_sigmoid(z):
    c = min(-z)
    return c + np.logaddexp(np.zeros(z.shape) - c, -z - c)


def negative_log_likelihood(X, theta):
    return -sum(-log_inv_sigmoid(X @ theta)) / X.shape[0]
