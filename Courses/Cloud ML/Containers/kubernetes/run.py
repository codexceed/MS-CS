import os

import torch

from mnist_app import app

if __name__ == "__main__":
    os.environ["CUDA"] = str(torch.cuda.is_available())
    app.run(host="127.0.0.1", port="5001", debug=True)
