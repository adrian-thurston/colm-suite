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
#
# Drop a txl*.tar.gz into the build context to get the indep test cases too;
# see the note further down.

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

# txl, optional. The 17 @LANG:indep test cases are translated into every host
# language by langtrans_<lang>.sh, which shells out to txl; without it runtests
# skips all 17 for *every* language, C and Go included, so it is the single
# largest coverage win available. It is not in apt and the binary distribution
# is not always to hand, so it is installed if and only if a tarball is sitting
# in the build context:
#
#   cp ~/installs/txl/txl10.5h.linux64.tar.gz package/
#   docker build -t ragel6-dev package/
#
# Anything matching txl*.tar.gz is picked up. Exactly one may be present; more
# than that is an error rather than a guess, since the wrong pick would quietly
# cost the indep tests. The tarball must match the build architecture: the
# plain "linux" ones are i386, the "linux64" ones x86_64. That is checked
# against the ELF header rather than by running the binary, because under
# buildx a foreign-arch binary may execute happily via qemu and only reveal
# itself later.
#
# Licence (COPYRIGHT.txt): free of charge for use by individuals, companies and
# institutions, and redistributable provided the distribution is kept entire
# and unmodified and nothing is charged for it -- so the tarball may simply be
# committed next to this file. Published work depending on it is asked to
# acknowledge TXL.
COPY . /tmp/ctx/
RUN set -eux; \
    set -- /tmp/ctx/txl*.tar.gz; \
    if [ ! -e "$1" ]; then \
        echo "no txl*.tar.gz in the build context;" \
             "the 17 indep test cases will be skipped"; \
    elif [ $# -gt 1 ]; then \
        echo "more than one txl tarball in the build context:" >&2; \
        for f; do echo "  $( basename "$f" )" >&2; done; \
        echo "leave only the one to install." >&2; \
        exit 1; \
    else \
        mkdir -p /opt/txl; \
        echo "installing $( basename "$1" )"; \
        tar --strip-components=1 -xzf "$1" -C /opt/txl; \
        case "$(uname -m)" in \
            x86_64)  want=3e00 ;; \
            aarch64) want=b700 ;; \
            i?86)    want=0300 ;; \
            *)       want= ;; \
        esac; \
        got="$( od -An -tx1 -j18 -N2 /opt/txl/bin/txl | tr -d ' \n' )"; \
        if [ -n "$want" ] && [ "$got" != "$want" ]; then \
            echo "txl is built for the wrong architecture: ELF e_machine" >&2; \
            echo "$got, need $want for $(uname -m). The plain 'linux'" >&2; \
            echo "tarballs are i386 (0300); use a linux64 one." >&2; \
            exit 1; \
        fi; \
        /opt/txl/bin/txl -help 2>&1 | grep -q "TXL processor" || \
            { echo "extracted txl does not run" >&2; exit 1; }; \
    fi; \
    rm -rf /tmp/ctx
# Ahead of the shipped path, and harmless when nothing was extracted. txl finds
# its lib/ relative to the binary, so no install step and no TXLLIB are needed.
ENV PATH=/opt/txl/bin:${PATH}
#
# NOTE: two further gaps are in the tree, not in this image. runtests dispatches
# on language twice and the second case has no obj-c arm, so those six tests
# never run and the suite exits 0 having done nothing. And ragel's D2 backend
# (-E) generates code gdc accepts cleanly -- only the D1-era .rl sources fail --
# so porting them is what restores D, on any base.

COPY --from=kelbt /usr/local/ /usr/local/
RUN ldconfig
