FROM rust:latest as builder

RUN set -eux; \
    apt-get update; \
    env DEBIAN_FRONTEND="noninteractive" \
    apt-get install -y --no-install-recommends \
    libhyperscan-dev

WORKDIR /work/

COPY Cargo.toml Cargo.lock /work/
COPY src /work/src/

RUN cargo install --path .

FROM ubuntu:20.04

RUN set -eux; \
    apt-get update; \
    apt-get install -y --no-install-recommends \
    libhyperscan5

COPY --from=builder /usr/local/cargo/bin/reproduce /usr/local/bin/

VOLUME /data
WORKDIR /data
ENTRYPOINT ["/usr/local/bin/reproduce"]
