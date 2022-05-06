import unittest
import multiclass
import nodes
import numpy as np
import test_utils

class TestNodes(unittest.TestCase):

    def test_SoftmaxNode(self):
        z = nodes.ValueNode(node_name="z")
        softmax_node = nodes.SoftmaxNode(z, "softmax")
        m = 4
        init_vals = {"z":np.random.randn(m)}

        max_rel_err = test_utils.test_node_backward(softmax_node, init_vals, delta=1e-7)
        max_allowed_rel_err = 1e-5
        self.assertTrue(max_rel_err < max_allowed_rel_err)

    def test_multiclass(self):
        estimator = multiclass.MulticlassClassifier()
        num_hidden_units = 4
        num_ftrs = 5
        num_class = 3
        input_vals = {"x": np.random.randn(num_ftrs)}
        outcome_vals = {"y": np.array(np.random.randint(low=0, high=num_class))}
        parameter_vals = {"W1": np.random.standard_normal((num_hidden_units, num_ftrs)),
                          "b1": np.random.standard_normal((num_hidden_units)),
                          "W2": np.random.standard_normal((num_class, num_hidden_units)),
                          "b2": np.array(np.random.randn(num_class)) }

        max_rel_err = test_utils.test_ComputationGraphFunction(estimator.graph, input_vals, outcome_vals, parameter_vals)
        max_allowed_rel_err = 1e-2
        self.assertTrue(max_rel_err < max_allowed_rel_err)



if __name__ == "__main__":
    unittest.main()
