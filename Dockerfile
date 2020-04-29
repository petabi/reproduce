FROM ubuntu:19.10 as builder

ARG RUSTUP_VERSION=1.20.2
ARG RUST_VERSION=1.41.0

RUN set -eux; \
    apt-get update; \
    apt-get install -y --no-install-recommends \
    ca-certificates \
    cmake \
    g++ \
    libhyperscan-dev \
    libpcap-dev \
    librdkafka-dev \
    libssl-dev \
    make \
    pkgconf \
    wget

ENV RUSTUP_HOME=/usr/local/rustup \
    CARGO_HOME=/usr/local/cargo \
    PATH=/usr/local/cargo/bin:$PATH

RUN RUST_ARCH='x86_64-unknown-linux-gnu'; \
    url="https://static.rust-lang.org/rustup/archive/${RUSTUP_VERSION}/${RUST_ARCH}/rustup-init"; \
    wget "$url"; \
    chmod +x rustup-init; \
    ./rustup-init -y --no-modify-path --default-toolchain ${RUST_VERSION}; \
    rm rustup-init

WORKDIR /work/

COPY Cargo.toml Cargo.lock CMakeLists.txt version.h.in /work/
COPY src /work/src/
COPY ext /work/ext/

RUN mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j

FROM ubuntu:19.10

RUN set -eux; \
    apt-get update; \
    apt-get install -y --no-install-recommends \
    libhyperscan5 \
    libpcap0.8 \
    librdkafka++1

COPY --from=builder /work/build/src/reproduce /usr/local/bin/

VOLUME /data
WORKDIR /data
ENTRYPOINT ["/usr/local/bin/reproduce"]
