import torch
import numpy as np

data = [[1,2], [3,4]]

# Initialize tensor
x_data = torch.tensor(data)

# Initialize tensor from numpy array
np_array = np.array(data)
x_np = torch.from_numpy(np_array)

# Generate a tensor of ones from pre-existing tensor shape
x_ones = torch.ones_like(x_data)
print(f"Ones Tensor: \n {x_ones} \n")

# Generate random tensor values f   rom pre-existing tensor shape
x_rand = torch.rand_like(x_data, dtype=torch.float) # overrides the datatype of x_data
print(f"Random Tensor: \n {x_rand} \n")

# Generate tensors from shape
shape = (2,3)
rand_tensor = torch.rand(shape)
ones_tensor = torch.ones(shape)
zeros_tensor = torch.zeros(shape)

print(f"Random Tensor: \n {rand_tensor} \n")
print(f"Ones Tensor: \n {ones_tensor} \n")
print(f"Zeros Tensor: \n {zeros_tensor} \n")

# Tensor attributes
tensor = torch.rand(3,4)

print(tensor)
print(f"Shape of tensor: {tensor.shape}")
print(f"Datatype of tensor: {tensor.dtype}")
print(f"Device tensor is stored on: {tensor.device}")