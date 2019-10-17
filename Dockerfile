FROM ubuntu:19.10 as builder
RUN set -eux; \
    apt-get update; \
    apt-get install -y --no-install-recommends \
    cmake \
    g++ \
    libhyperscan-dev \
    libmsgpack-dev \
    libpcap-dev \
    librdkafka-dev \
    make
COPY ./ /work/
WORKDIR /work/
RUN mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make

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
