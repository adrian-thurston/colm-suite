# syntax=docker/dockerfile:1
#
# Development environment for Ragel 6.x: the C/C++ toolchain, autotools, a
# bootstrap ragel and kelbt for regenerating the parsers, latex and fig2dev for
# the guide, and Go for the test suite. Enough to build the tree, run the tests
# and roll a release (package/release.sh).
#
#   docker build -t ragel6-dev package/
#
# Build args:
#   KELBT_VERSION   kelbt release to fetch  (default 0.16)
#   UBUNTU_TAG      base image              (default 18.04; 22.04 works)
#   BUILD_PLATFORM  build architecture      (default linux/amd64)

ARG UBUNTU_TAG=18.04
# amd64 by default: ragel/kelbt are x86-era C++ that only compile where char is
# signed, so they fail on arm64. To build native arm64, set BUILD_PLATFORM to
# linux/arm64 and force signed char (add ENV CFLAGS=-fsigned-char
# CXXFLAGS=-fsigned-char to the kelbt and dev stages).
ARG BUILD_PLATFORM=linux/amd64

# Base: C/C++ toolchain, autotools, bootstrap ragel. 18.04 suits the 6.10 era;
# 22.04 also works (--build-arg UBUNTU_TAG=22.04).
FROM --platform=${BUILD_PLATFORM} ubuntu:${UBUNTU_TAG} AS base
ENV DEBIAN_FRONTEND=noninteractive
# Fall back to old-releases for EOL bases (handles amd64 and arm64 mirror paths).
RUN set -eux; \
    apt-get update 2>/dev/null || { \
        sed -i -e 's|//archive.ubuntu.com/ubuntu|//old-releases.ubuntu.com/ubuntu|g' \
               -e 's|//security.ubuntu.com/ubuntu|//old-releases.ubuntu.com/ubuntu|g' \
               -e 's|//ports.ubuntu.com/ubuntu-ports|//old-releases.ubuntu.com/ubuntu-ports|g' \
               /etc/apt/sources.list; \
        apt-get update; \
    }; \
    apt-get install -y --no-install-recommends \
        build-essential autoconf automake ragel git wget ca-certificates; \
    rm -rf /var/lib/apt/lists/*

# kelbt: the only build dependency not in apt.
FROM base AS kelbt
ARG KELBT_VERSION=0.16
WORKDIR /tmp/kelbt
RUN wget -q "https://www.colm.net/files/kelbt/kelbt-${KELBT_VERSION}.tar.gz" \
    && tar --strip-components=1 -xf "kelbt-${KELBT_VERSION}.tar.gz" \
    && ./configure \
    && make \
    && make install \
    && ldconfig

# Add what the manual and the test suite need on top of the base toolchain.
FROM base AS dev
# Manual toolchain: pdflatex (+ latex-extra for comment.sty), the fig->pdf
# converter (fig2dev on 20.04+, transfig on 18.04), and ghostscript for it.
RUN set -eux; \
    apt-get update; \
    apt-get install -y --no-install-recommends \
        texlive-latex-base texlive-latex-recommended texlive-latex-extra \
        texlive-fonts-recommended ghostscript; \
    apt-get install -y --no-install-recommends fig2dev \
        || apt-get install -y --no-install-recommends transfig; \
    rm -rf /var/lib/apt/lists/*

# Test toolchain: Go (C/C++ are already in base; the suite auto-skips any
# language whose compiler is absent).
RUN set -eux; \
    apt-get update; \
    apt-get install -y --no-install-recommends golang-go; \
    rm -rf /var/lib/apt/lists/*

COPY --from=kelbt /usr/local/ /usr/local/
RUN ldconfig
