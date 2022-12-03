FROM pytorch/pytorch:1.13.0-cuda11.6-cudnn8-runtime
COPY . /app
WORKDIR /app
RUN pip3 install -r requirements.txt
EXPOSE 5000
ENTRYPOINT ["gunicorn", "--config", "gunicorn_config.py", "run:app"]