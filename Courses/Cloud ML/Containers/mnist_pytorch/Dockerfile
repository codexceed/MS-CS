# Start with a Linux micro-container to keep the image tiny
FROM armswdev/pytorch-arm-neoverse:r22.10-torch-1.12.0-openblas

# Document who is responsible for this image
MAINTAINER Sarthak Joshi "sj2810@nyu.edu"

# Set up a working folder and install the pre-reqs
WORKDIR /mnist
ADD requirements.txt /mnist
RUN pip install -r requirements.txt

# Add the code as the last Docker layer because it changes the most
ADD mnist_pytorch.py /mnist

# Run mnist
ENTRYPOINT [ "python" ]
CMD [ "mnist_pytorch.py" ]


