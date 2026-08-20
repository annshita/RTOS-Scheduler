# =============================================================================
# Stage 1 — Build the C++ rtsched binary
# =============================================================================
FROM gcc:12 AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
        cmake \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY CMakeLists.txt .
COPY include/   include/
COPY src/        src/
COPY cli/        cli/
COPY tests/      tests/

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release \
 && cmake --build build --target rtsched -j"$(nproc)"

# =============================================================================
# Stage 2 — Node.js runtime
# =============================================================================
FROM node:20-slim

# Copy the compiled binary
COPY --from=builder /src/build/rtsched /usr/local/bin/rtsched
RUN chmod +x /usr/local/bin/rtsched

WORKDIR /app

# Install server dependencies first (layer-cached unless package.json changes)
COPY server/package.json server/
RUN cd server && npm ci --omit=dev

# Copy application files
COPY server/   server/
COPY web/      web/
COPY examples/ examples/

# Tell the server where to find the binary and what port to use
ENV BINARY_PATH=/usr/local/bin/rtsched
ENV PORT=3000

EXPOSE 3000

CMD ["node", "server/index.js"]
