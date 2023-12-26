FROM golang:1.17-bullseye AS builder

RUN apt update && apt install -y cmake libcurl4-openssl-dev \
    libpq-dev rapidjson-dev unixodbc unixodbc-dev libsqlite3-dev

WORKDIR /usr/src/ldp
COPY . /usr/src/ldp

RUN ./version.sh && \
    mkdir -p build && \
    cd build && \
    cmake -DPostgreSQL_TYPE_INCLUDE_DIR=/usr/include/postgresql .. && \
    make


FROM debian:bullseye-slim

LABEL org.opencontainers.image.source="https://github.com/library-data-platform/ldp"
ENV DATADIR=/var/lib/ldp

COPY --from=builder /usr/src/ldp/build/ldp /usr/local/bin/ldp
COPY docker-entrypoint.sh /usr/local/bin/docker-entrypoint.sh

RUN apt update && apt install -y && apt install -y libcurl4 libpq5 && \
    mkdir $DATADIR && \
    chmod +x /usr/local/bin/docker-entrypoint.sh

VOLUME $DATADIR

ENTRYPOINT ["/usr/local/bin/docker-entrypoint.sh"]

