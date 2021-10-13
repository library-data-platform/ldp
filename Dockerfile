FROM golang:1.17-bullseye AS builder

RUN apt update && apt install -y cmake libcurl4-openssl-dev postgresql-server-dev-all \
    libpq-dev rapidjson-dev unixodbc unixodbc-dev libsqlite3-dev

WORKDIR /usr/src/ldp
COPY . /usr/src/ldp

RUN mkdir -p build && \ 
    cd build && \
    cmake -DPostgreSQL_TYPE_INCLUDE_DIR=/usr/include/postgresql .. && \
    make -j 6


FROM debian:bullseye-slim

ENV DATADIR=/var/lib/ldp

COPY --from=builder /usr/src/ldp/build/ldp /usr/local/bin/ldp
COPY docker-entrypoint.sh /usr/local/bin/docker-entrypoint.sh

RUN apt update && apt install -y && apt install -y libcurl4 libpq5 && \
    mkdir $DATADIR && \
    chmod +x /usr/local/bin/docker-entrypoint.sh

VOLUME $DATADIR

ENTRYPOINT ["/usr/local/bin/docker-entrypoint.sh"]

