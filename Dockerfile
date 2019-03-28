FROM ubuntu:18.04 as pkgsrc
ENV \
    PATH=$PATH:/usr/pkg/bin:/usr/pkg/sbin \
    PKG_PATH=https://pkg.petabi.com/Ubuntu-18.04/All
RUN \
    apt-get update -yqq && \
    apt-get install -yqq build-essential curl
RUN \
    curl -SL https://pkg.petabi.com/Ubuntu-18.04/bootstrap.tar.gz \
    | tar -zxp -C / && \
    pkg_add pkg_alternatives libpcap-1.8.1 librdkafka-0.11.4 msgpack-3.1.1 googletest-1.8.1 hyperscan-5.0.0

FROM pkgsrc as builder
RUN \
    apt-get install -y cmake
COPY ./ /work/
RUN ls -la /work/
WORKDIR /work/
RUN mkdir build && cd build && cmake .. && make

FROM pkgsrc as reproduce
ARG CONFIG
COPY --from=builder /work/build/src/reproduce /usr/pkg/bin/.
VOLUME  /data
WORKDIR /data
ENTRYPOINT ["reproduce"]
