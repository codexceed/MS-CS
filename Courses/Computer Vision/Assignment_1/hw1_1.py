from __future__ import print_function
import argparse
import torch
import torch.nn as nn
import torch.nn.functional as F
import torch.optim as optim
from torchvision import datasets, transforms, utils
from torch.autograd import Variable

import matplotlib.pyplot as plt
import numpy as np

plt.figure(figsize=(3,3))

def imshow(img):
    img = img / 2 + 0.5     # unnormalize
    npimg = img.numpy()
    plt.imshow(np.transpose(npimg, (1, 2, 0)))
    plt.show()


# options
dataset = 'cifar10' # options: 'mnist' | 'cifar10'
batch_size = 64   # input batch size for training
epochs = 10       # number of epochs to train
lr = 0.02        # learning rate
limit = False
device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")

# Data Loading
# Warning: this cell might take some time when you run it for the first time,
#          because it will download the datasets from the internet
if dataset == 'mnist':
    data_transform = transforms.Compose([
        transforms.ToTensor(),
        transforms.Normalize((0.1307,), (0.3081,))
    ])
    trainset = datasets.MNIST(root='.', train=True, download=True, transform=data_transform)
    testset = datasets.MNIST(root='.', train=False, download=True, transform=data_transform)

elif dataset == 'cifar10':
    data_transform = transforms.Compose([
        transforms.ToTensor(),
        transforms.Normalize((0.5, 0.5, 0.5), (0.5, 0.5, 0.5)),
    ])
    trainset = datasets.CIFAR10(root='.', train=True, download=True, transform=data_transform)
    testset = datasets.CIFAR10(root='.', train=False, download=True, transform=data_transform)

# Here's where we limit the dataset.
if limit:
    trainset.data = trainset.data[:50]

train_loader = torch.utils.data.DataLoader(trainset, batch_size=batch_size, shuffle=True, num_workers=0)
test_loader  = torch.utils.data.DataLoader(testset, batch_size=batch_size, shuffle=False, num_workers=0)

# Visualize images
dataiter = iter(train_loader)
images, labels = dataiter.next()
print(images.shape)
imshow(utils.make_grid(images))

## network and optimizer
if dataset == 'mnist':
    num_inputs = 784
elif dataset == 'cifar10':
    num_inputs = 3072

num_outputs = 10  # same for both CIFAR10 and MNIST, both have 10 classes as outputs

# # MNIST Default
# class Net(nn.Module):
#     def __init__(self, num_inputs, num_outputs):
#         super(Net, self).__init__()
#         self.linear = nn.Linear(num_inputs, num_outputs)
#
#     def forward(self, input):
#         input = input.view(-1, num_inputs) # reshape input to batch x num_inputs
#         output = self.linear(input)
#         return output

# MNIST
# class Net(nn.Module):
#     def __init__(self, num_inputs, num_outputs):
#         super(Net, self).__init__()
#         self.linear1 = nn.Linear(num_inputs, 1000)
#         self.tanh = nn.Tanh()
#         self.linear2 = nn.Linear(1000, num_outputs)

#     def forward(self, input):
#         input = input.view(-1, num_inputs) # reshape input to batch x num_inputs
#         output = self.linear1(input)
#         output = self.tanh(output)
#         output = self.linear2(output)
#         return output

# CIFAR10 Omtimized
class Net(nn.Module):
    def __init__(self, num_inputs, num_outputs):
        super(Net, self).__init__()
        self.conv1 = nn.Conv2d(3, 16, 5)
        self.tanh = nn.Tanh()
        self.pool1 = nn.MaxPool2d(2, 2)
        self.conv2 = nn.Conv2d(16, 128, 5)
        self.flatten = nn.Flatten()
        self.linear1 = nn.Linear(3200, 64)
        self.linear2 = nn.Linear(64, num_outputs)

    def forward(self, input):
        # input = input.view(-1, num_inputs) # reshape input to batch x num_inputs
        output = self.conv1(input)
        output = self.tanh(output)
        output = self.pool1(output)
        output = self.conv2(output)
        output = self.tanh(output)
        output = self.pool1(output)
        output = self.flatten(output)
        output = self.linear1(output)
        output = self.tanh(output)
        output = self.linear2(output)
        return output


network = Net(num_inputs, num_outputs).to(device)
optimizer = optim.SGD(network.parameters(), lr=lr)


# Define training and test functions
def train(epoch):
    network.train()
    count = 0
    for i in range(epoch):
      for batch_idx, (data, target) in enumerate(train_loader):
          data, target = Variable(data).to(device), Variable(target).to(device)
          optimizer.zero_grad()
          output = network(data)
          loss = F.cross_entropy(output, target)
          loss.backward()
          optimizer.step()
          if batch_idx % 100 == 0:
              print('Train Epoch: {} [{}/{} ({:.0f}%)]\tLoss: {:.6f}'.format(
                  i, batch_idx * len(data), len(train_loader.dataset),
                  100. * batch_idx / len(train_loader), loss.item()))
          count += 1
          if limit and count == 50:
            break

def test():
    network.eval()
    test_loss = 0
    correct = 0
    for data, target in test_loader:
        data = data.to(device)
        target = target.to(device)
        #data, target = Variable(data, volatile=True), Variable(target)
        output = network(data)
        test_loss += F.cross_entropy(output, target, reduction='sum').item() # sum up batch loss
        #test_loss += F.cross_entropy(output, target, sum=True).item() # sum up batch loss
        pred = output.data.max(1, keepdim=True)[1] # get the index of the max log-probability
        correct += pred.eq(target.data.view_as(pred)).cpu().sum()

    test_loss /= len(test_loader.dataset)
    print('\nTest set: Average loss: {:.4f}, Accuracy: {}/{} ({:.0f}%)\n'.format(
        test_loss, correct, len(test_loader.dataset),
        100. * correct / len(test_loader.dataset)))


# Train and test here
train(epochs)
test()

image = network.linear.weight

# CIFAR10 net weight
# image = network.linear2.weight

imshow(utils.make_grid(image.cpu()))

