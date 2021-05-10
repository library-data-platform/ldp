FROM debian:buster-slim

ENV DATADIR=/var/lib/ldp

RUN mkdir -p $DATADIR

RUN apt update -y && apt install -y libcurl4-openssl-dev

COPY ./build/ldp /bin/ldp

ENTRYPOINT ["ldp"]
