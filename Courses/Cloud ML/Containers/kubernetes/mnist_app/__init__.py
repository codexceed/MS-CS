"""Initializes the web app."""
from flask import Flask

app = Flask(__name__)

from mnist_app import routes  # NOQA
