FROM rust:latest as builder

WORKDIR /work/

COPY Cargo.toml Cargo.lock /work/
COPY src /work/src/

RUN cargo install --path .

FROM ubuntu:20.04

COPY --from=builder /usr/local/cargo/bin/reproduce /usr/local/bin/

VOLUME /data
WORKDIR /data
ENTRYPOINT ["/usr/local/bin/reproduce"]
