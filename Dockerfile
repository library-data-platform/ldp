FROM debian:buster-slim

ENV DATADIR=/var/lib/ldp

RUN mkdir -p $DATADIR

RUN apt update -y && apt install -y unixodbc odbc-postgresql \ 
  odbc-postgresql libcurl4-openssl-dev

COPY ./build/ldp /bin/ldp

ENTRYPOINT ["ldp"]
