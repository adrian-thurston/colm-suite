#
# This dockerfile demonstrates building the Colm Suite (Colm + Ragel) from a
# release tarball.
#

FROM ubuntu:focal AS build

ENV DEBIAN_FRONTEND="noninteractive"

RUN apt-get update && apt-get install -y \
	gpg g++ gcc make curl

RUN curl https://www.colm.net/files/thurston.asc | gpg --import -

WORKDIR /build
ENV COLM_SUITE_VERSION=0.15.0-pre.1
RUN curl -O https://www.colm.net/files/colm-suite/colm-suite-${COLM_SUITE_VERSION}.tar.gz
RUN curl -O https://www.colm.net/files/colm-suite/colm-suite-${COLM_SUITE_VERSION}.tar.gz.asc
RUN gpg --verify colm-suite-${COLM_SUITE_VERSION}.tar.gz.asc colm-suite-${COLM_SUITE_VERSION}.tar.gz
RUN tar -zxvf colm-suite-${COLM_SUITE_VERSION}.tar.gz
WORKDIR /build/colm-suite-${COLM_SUITE_VERSION}
RUN ./configure --prefix=/opt/colm-suite --disable-manual
RUN make
RUN make install
