FROM debian:trixie-slim

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    pkg-config \
    ca-certificates \
    qt6-base-dev \
    qt6-base-dev-tools \
    qt6-tools-dev \
    libxmlrpc-c++9-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy project files
COPY . .

# Build (limit parallelism to avoid OOM)
RUN mkdir -p build && cd build && cmake .. && make -j2

# Default command: run unit tests
CMD ["sh", "-c", "./build/rssani_tests_values && ./build/rssani_tests_mail && ./build/rssani_tests_rss"]
