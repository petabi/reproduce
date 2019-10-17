FROM ubuntu:19.10 as builder
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
    PATH=/usr/local/cargo/bin:$PATH \
    RUST_VERSION=1.38.0
RUN rustArch='x86_64-unknown-linux-gnu'; \
    url="https://static.rust-lang.org/rustup/archive/1.20.2/${rustArch}/rustup-init"; \
    wget "$url"; \
    chmod +x rustup-init; \
    ./rustup-init -y --no-modify-path --default-toolchain $RUST_VERSION; \
    rm rustup-init
COPY Cargo.toml Cargo.lock CMakeLists.txt version.h.in /work/
COPY src /work/src/
COPY ext /work/ext/
WORKDIR /work/
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
