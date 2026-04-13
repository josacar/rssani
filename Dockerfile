FROM debian:trixie-slim

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
    libgrpc++-dev \
    libprotobuf-dev \
    protobuf-compiler \
    protobuf-compiler-grpc \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy only source files needed for the build
COPY CMakeLists.txt .
COPY rssani.proto .
COPY mailsender.cpp mailsender.h .
COPY myircsession.cpp myircsession.h .
COPY rss_lite.cpp rss_lite.h .
COPY rssani_lite.cpp rssani_lite.h .
COPY grpc_server.cpp grpc_server.h .
COPY main.cpp .
COPY values.h .
COPY rssani_en_US.ts .
COPY tests/ tests/

# Build the project
RUN mkdir -p build && cd build && cmake .. && make -j2

# Default command: run all unit tests
CMD ["sh", "-c", "cd build && ./rssani_tests_values && ./rssani_tests_mail && ./rssani_tests_rss && ./rssani_tests_rssani && ./rssani_tests_irc"]
