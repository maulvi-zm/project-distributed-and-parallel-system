FROM ubuntu:latest

RUN apt-get update && apt-get install -y \
    openmpi-bin \
    openmpi-common \
    libopenmpi-dev \
    openssh-server \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /mpi
COPY . /mpi

RUN mkdir -p /root/.ssh

CMD ["bash", "-c", "service ssh start && exec /bin/bash"]
