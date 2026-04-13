FROM debian:trixie-slim AS base

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

# ============================================================================
# Test build: builds everything including test executables
# ============================================================================
FROM base AS test-builder

# Copy test sources
COPY tests/ tests/

# Build the project with tests
RUN mkdir -p build && cd build && cmake .. && make -j2

# ============================================================================
# Release build: builds only the rssani binary (no tests)
# ============================================================================
FROM base AS release-builder

# Build the project (tests won't be built via CMake option)
RUN mkdir -p build && cd build && cmake -DRSSANI_BUILD_TESTS=OFF .. && make -j2

# ============================================================================
# Test stage: run unit tests
# ============================================================================
FROM debian:trixie-slim AS test

ENV DEBIAN_FRONTEND=noninteractive

# Install runtime dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    qt6-base-dev \
    libgrpc++-dev \
    libprotobuf-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy built artifacts from test-builder
COPY --from=test-builder /app/build /app/build

# Default command: run all unit tests
CMD ["sh", "-c", "cd build && ./rssani_tests_values && ./rssani_tests_mail && ./rssani_tests_rss && ./rssani_tests_rssani && ./rssani_tests_irc"]

# ============================================================================
# Release stage: minimal image with only the rssani binary
# ============================================================================
FROM debian:trixie-slim AS release

ENV DEBIAN_FRONTEND=noninteractive
ENV LD_LIBRARY_PATH=/app/lib

# Install runtime dependencies only
RUN apt-get update && apt-get install -y --no-install-recommends \
    qt6-base-dev \
    libgrpc++-dev \
    libprotobuf-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy the rssani binary and libirc shared libraries from release-builder
COPY --from=release-builder /app/build/rssani .
COPY --from=release-builder /app/build/deps/libirc/lib/libirc.so /app/lib/libirc.so
COPY --from=release-builder /app/build/deps/libirc/lib/libircclient.so /app/lib/libircclient.so

# Default command: run the rssani binary
CMD ["./rssani"]
