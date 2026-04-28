FROM alpine:latest AS base

ENV LC_ALL=C.UTF-8

RUN apk add --no-cache \
    build-base \
    cmake \
    git \
    pkgconfig \
    ca-certificates \
    qt6-qtbase-dev \
    qt6-qttools-dev \
    grpc-dev \
    protobuf-dev \
    protoc

WORKDIR /app

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

COPY tests/ tests/

RUN mkdir -p build && cd build && cmake .. && make -j$(nproc)

# ============================================================================
# Release build: builds only the rssani binary (no tests)
# ============================================================================
FROM base AS release-builder

RUN mkdir -p build && cd build && cmake -DRSSANI_BUILD_TESTS=OFF .. && make -j$(nproc)

# ============================================================================
# Test stage: run unit tests
# ============================================================================
FROM alpine:latest AS test

ENV LC_ALL=C.UTF-8
ENV LD_LIBRARY_PATH=/app/lib

RUN apk add --no-cache \
    qt6-qtbase \
    grpc \
    protobuf

WORKDIR /app

COPY --from=test-builder /app/build /app/build

CMD ["sh", "-c", "cd build && ./rssani_tests_values && ./rssani_tests_mail && ./rssani_tests_rss && ./rssani_tests_rssani && ./rssani_tests_irc"]

# ============================================================================
# Release stage: minimal image with only the rssani binary
# ============================================================================
FROM alpine:latest AS release

ENV LC_ALL=C.UTF-8
ENV LD_LIBRARY_PATH=/app/lib

RUN apk add --no-cache \
    qt6-qtbase \
    grpc \
    protobuf

WORKDIR /app

COPY --from=release-builder /app/build/rssani .
COPY --from=release-builder /app/build/deps/libirc/lib/libirc.so /app/lib/libirc.so
COPY --from=release-builder /app/build/deps/libirc/lib/libircclient.so /app/lib/libircclient.so

CMD ["./rssani"]