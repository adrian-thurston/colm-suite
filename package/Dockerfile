# syntax=docker/dockerfile:1
#
# Development environment for Ragel 6.x: the C/C++ toolchain, autotools, a
# bootstrap ragel and kelbt for regenerating the parsers, latex and fig2dev for
# the guide, and the host-language compilers the test suite drives. Enough to
# build the tree, run the tests and roll a release (package/release.sh).
#
#   docker build -t ragel6-dev package/
#
# Build args:
#   KELBT_VERSION   kelbt release to fetch  (default 0.16)
#   UBUNTU_TAG      base image              (default 16.04; 18.04-22.04 work)
#   BUILD_PLATFORM  build architecture      (default linux/amd64)

ARG UBUNTU_TAG=16.04
# amd64 by default: ragel/kelbt are x86-era C++ that only compile where char is
# signed, so they fail on arm64. To build native arm64, set BUILD_PLATFORM to
# linux/arm64 and force signed char (add ENV CFLAGS=-fsigned-char
# CXXFLAGS=-fsigned-char to the kelbt and dev stages).
ARG BUILD_PLATFORM=linux/amd64

# Base: C/C++ toolchain, autotools, bootstrap ragel. 16.04 is the last release
# predating 6.10 (Mar 2017), so it approximates the toolchain of that era; newer
# bases also work (--build-arg UBUNTU_TAG=22.04).
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

# Test toolchain: the host languages test/runtests can drive. C/C++ are already
# in base; the suite silently skips any language whose compiler is absent, so a
# gap here costs coverage without failing the run.
#
#   gobjc        obj-C. configure only sets GOBJC if a test compile succeeds,
#                and plain build-essential has no cc1obj.
#   golang-go    Go.
#   default-jdk  Java (javac). Not openjdk-8-jdk, which is absent on bases
#                older than xenial; default-jdk resolves on all of them.
#   ruby         Ruby.
#   mono-devel   C#. No native test cases -- only pays off alongside txl.
#   gdc          D. Currently buys nothing: runtests drives the D1 backend (-D)
#                and every gdc is D2. Kept so that a -E/D2 test path, once the
#                four D test cases are ported, needs no image change.
#
# Not covered: OCaml, which ragel can target but runtests has no path for.
RUN set -eux; \
    apt-get update; \
    apt-get install -y --no-install-recommends \
        gobjc golang-go default-jdk ruby mono-devel gdc; \
    rm -rf /var/lib/apt/lists/*

# configure checks for the C# compiler under its old name (AC_CHECK_PROG GMCS),
# which newer mono dropped in favour of mcs. The name also gates runtests into
# prefixing the binary with the mono launcher, so make sure it resolves -- but
# only when mono did not already install a real gmcs, as older versions do.
RUN command -v gmcs >/dev/null || ln -sf mcs /usr/bin/gmcs

# NOTE: the 17 @LANG:indep test cases are translated into every host language by
# langtrans_<lang>.sh, which needs txl -- without it runtests skips all 17 for
# *every* language, C and Go included. txl is not packaged (tarball from txl.ca,
# and its licence is non-commercial-use only), so it is deliberately not
# installed here. Adding it is the single largest coverage win available.
#
# NOTE: two further gaps are in the tree, not in this image. runtests dispatches
# on language twice and the second case has no obj-c arm, so those six tests
# never run and the suite exits 0 having done nothing. And ragel's D2 backend
# (-E) generates code gdc accepts cleanly -- only the D1-era .rl sources fail --
# so porting them is what restores D, on any base.

COPY --from=kelbt /usr/local/ /usr/local/
RUN ldconfig
