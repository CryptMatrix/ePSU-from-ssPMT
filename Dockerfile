FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

WORKDIR /home

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    openssl \
    build-essential \
    cmake \
    git \
    libtool \
    autoconf \
    automake \
    pkg-config \
    iproute2 \
    python3 \
    sudo \
    nasm \
    libssl-dev \
    libgmp-dev \
    wget \
    libfmt-dev \
    && update-ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY . /home/ePSU-ssPMT

RUN cd /home/ePSU-ssPMT && \
    cd ePSU_fast && \
    ./setup.sh

RUN cd /home/ePSU-ssPMT && \
    cd ePSU_low && \
    ./setup.sh

WORKDIR /home/ePSU-ssPMT
