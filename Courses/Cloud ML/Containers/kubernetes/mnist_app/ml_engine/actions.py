import os
from pathlib import Path

import torch
import torch.optim as optim
from PIL import Image
from torch.optim.lr_scheduler import StepLR
from torchvision import datasets, transforms

from .utils import Net, train, validate


MODEL_FILE = "mnist_cnn.pt"


def _get_device():
    use_cuda = os.getenv("CUDA") == "True"
    
    if use_cuda and torch.cuda.is_available():
        device = torch.device("cuda")
    else:
        device = torch.device("cpu")

    return device


def train_model():
    """Train the neural net on MNIST."""
    seed = 1

    torch.manual_seed(seed)

    device = _get_device()

    train_kwargs = {"batch_size": 64}
    test_kwargs = {"batch_size": 1000}
    if device == torch.device("cuda"):
        cuda_kwargs = {"num_workers": 1, "pin_memory": True, "shuffle": True}
        train_kwargs.update(cuda_kwargs)
        test_kwargs.update(cuda_kwargs)

    transform = transforms.Compose(
        [transforms.ToTensor(), transforms.Normalize((0.1307,), (0.3081,))]
    )
    dataset1 = datasets.MNIST("../data", train=True, download=True, transform=transform)
    dataset2 = datasets.MNIST("../data", train=False, transform=transform)
    train_loader = torch.utils.data.DataLoader(dataset1, **train_kwargs)
    test_loader = torch.utils.data.DataLoader(dataset2, **test_kwargs)

    model = Net().to(device)
    optimizer = optim.Adadelta(model.parameters(), lr=1.0)

    scheduler = StepLR(optimizer, step_size=1, gamma=0.7)
    for epoch in range(1, 15):
        train(model, device, train_loader, optimizer, epoch)
        validate(model, device, test_loader)
        scheduler.step()

    torch.save(model.state_dict(), MODEL_FILE)


def infer_digit(image_path: str) -> int:
    """
    Infer the digit from given image.

    Args:
        image_path: Path to image file.

    Returns:
        Classified digit as integer.
    """
    if not (Path("./") / MODEL_FILE).exists():
        train_model()

    device = _get_device()

    model = Net().to(device)
    model.load_state_dict(torch.load(MODEL_FILE))

    transform = transforms.Compose(
        [transforms.ToTensor(), transforms.Normalize((0.1307,), (0.3081,))]
    )

    img = transform(Image.open(image_path))[0].reshape(1, 1, 28, 28).to(device)

    output = model(img)
    return int(output.argmax(dim=1)[0])
