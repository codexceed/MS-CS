"""Defines API endpoints."""
import os
from typing import Any

from flask import abort, make_response, request

from mnist_app import app

from .ml_engine.actions import MODEL_FILE, infer_digit, train_model


@app.route("/status", methods=["GET"])
def status() -> Any:
    """Get service status"""
    response = make_response("ML Service up", 201)
    return response


@app.route("/train", methods=["POST"])
def train_mnist() -> Any:
    """Train model on MNIST."""
    train_model()
    response = make_response("Training completed!", 201)
    return response


@app.route("/classify", methods=["POST"])
def get_digit() -> Any:
    """Return the digit identified in the image."""
    uploaded_file = request.files["file"]
    if uploaded_file.filename != "":
        if not os.path.exists(MODEL_FILE):
            response = make_response(
                "Model not trained yet. Please trigger training and retry.", 500
            )
            return response

        uploaded_file.save(uploaded_file.filename)
        digit = infer_digit(uploaded_file.filename)
        os.remove(uploaded_file.filename)

        return {"digit": digit}

    abort(400)
