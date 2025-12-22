FROM ubuntu:22.04 AS builder
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    qt6-base-dev \
    libpqxx-dev \
    ca-certificates \
    libcurl4-openssl-dev \
    libgl1-mesa-dev \
    libxkbcommon-x11-0 \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY . .
RUN mkdir build && cd build && \
    cmake .. && \
    make -j$(nproc)
CMD ["./build/bdu_app"]