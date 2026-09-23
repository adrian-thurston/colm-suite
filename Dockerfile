# syntax=docker/dockerfile:1
#
# Development environment for the mainline colm suite (colm + ragel): the
# C/C++ toolchain, autotools and cmake, enough to build the tree either
# way, plus the host language toolchains the ragel test suite drives.
# Unlike the ragel-6 image there is no kelbt and no bootstrap ragel: colm
# builds first and then generates ragel's parsers, which are written in
# colm.
#
#   docker build -t colm-suite-dev .
#
# Then, from a checkout mounted into the container:
#
#   docker run --rm -it -v "$PWD":/devel/colm -w /devel/colm colm-suite-dev
#   ./autogen.sh && ./configure && make && make check
#
# or, for the cmake build, which needs no autotools step:
#
#   cmake -S . -B build && cmake --build build
#
# Build args:
#   UBUNTU_TAG     base image (default 26.04, i.e. the current Ubuntu LTS);
#                  pin with e.g. --build-arg UBUNTU_TAG=24.04
#   ZIG_VERSION    zig release to install from ziglang.org/download (default
#                  0.16.0); pin with e.g. --build-arg ZIG_VERSION=0.15.1
#   JULIA_VERSION  julia release to install from julialang.org/downloads
#                  (default 1.12.7)
#   CRACK_VERSION  crack release to build from crack-lang.org/download.html
#                  (default 1.7); built on x86-64 only
#   LLVM_VERSION   llvm release to build crack against (default 3.3); crack
#                  does not build against anything newer
#
# Host languages covered, as the test suite exercises them: C, C++, Objective-C
# (gnustep), D, Java, Ruby, C#, Go, OCaml, Rust, Julia and Zig everywhere, plus
# crack on x86-64, where it is built from source because no distribution
# packages it. An arm64 image gets neither crack, whose LLVM is configured for
# the x86 target here, nor asm, which needs no toolchain beyond $CC but is
# x86-64 only. Configure and the test suite skip both silently when they are
# absent. The documentation toolchain and gpg are installed too, so release
# work can happen in the container: building the manuals, signing and
# verifying tarballs (a key must be mounted in; the image carries none).

ARG UBUNTU_TAG=26.04
ARG ZIG_VERSION=0.16.0
ARG JULIA_VERSION=1.12.7
ARG CRACK_VERSION=1.7
ARG LLVM_VERSION=3.3

# No platform pin: unlike ragel 6.x, mainline builds natively on both amd64
# and arm64.
FROM ubuntu:${UBUNTU_TAG} AS dev
ENV DEBIAN_FRONTEND=noninteractive

# By default the container will run in the C locale. Some ragel contributions
# are in other charsets so ensure those display correctly when working in
# containers built with this dockerfile. C.UTF-8 is built into glibc, so no
# locales package is needed. LANG only: leave LC_ALL unset so the user can
# still override individual categories.
ENV LANG=C.UTF-8

RUN set -eux; \
    apt-get update; \
    apt-get install -y --no-install-recommends \
        build-essential autoconf automake libtool cmake git ca-certificates \
        curl xz-utils; \
    rm -rf /var/lib/apt/lists/*

# Test suite host languages available from the ubuntu archive.
#
#   gobjc, gnustep-make, libgnustep-base-dev
#       objective-c: the cc1obj front end, gnustep-config (which configure
#       probes for) and the gnustep-base headers and library the tests link.
#   golang-go       go
#   default-jdk     java: javac to compile, the java launcher to run
#   ruby            ruby
#   ocaml           the ocaml toplevel; the tests are interpreted, not compiled
#   rustc           rust
#   gdc             d. The archive ships gdc-11 through gdc-16 plus this
#                   unversioned front, which is what configure's fallback probe
#                   picks up.
#   mono-devel      c#: mcs to compile; pulls in mono-runtime to run. Ubuntu
#                   26.04 repackaged mono (6.14, from the winehq-maintained
#                   source) under these names; the old mono-mcs is gone.
#                   Do not add the mono-project.com repository: it is frozen at
#                   6.12 and its packages conflict with the archive's.
RUN set -eux; \
    apt-get update; \
    apt-get install -y --no-install-recommends \
        gobjc gnustep-make libgnustep-base-dev \
        golang-go default-jdk ruby ocaml rustc gdc mono-devel; \
    rm -rf /var/lib/apt/lists/*

# Documentation toolchain and release tooling.
#
#   asciidoc          asciidoc and a2x, plus the icons the colm manual pulls
#                     from /usr/share/asciidoc/icons; configure requires it
#                     for --enable-manual
#   python3-pygments  pygmentize, the manual's source highlighter; also
#                     required by configure for --enable-manual
#   fig2dev           the .fig diagrams in the ragel guide
#   dblatex           the a2x pdf backend, for ragel-guide.pdf; by far the
#                     largest piece, it pulls in tex live
#   gnupg             gpg, for signing and verifying release tarballs
#
# Recommends are left on here: dblatex and asciidoc lean on recommended
# tex and font packages that are painful to enumerate by hand.
RUN set -eux; \
    apt-get update; \
    apt-get install -y \
        asciidoc dblatex fig2dev python3-pygments gnupg; \
    rm -rf /var/lib/apt/lists/*

# Zig, from the upstream binary tarballs. Not needed to build the tree; it is
# here as a host language toolchain (and as a drop-in C/C++ cross compiler via
# "zig cc"). Shasums are those published in ziglang.org/download/index.json for
# ZIG_VERSION, so bumping the version means bumping these too.
ARG ZIG_VERSION
ARG ZIG_SHA256_X86_64=70e49664a74374b48b51e6f3fdfbf437f6395d42509050588bd49abe52ba3d00
ARG ZIG_SHA256_AARCH64=ea4b09bfb22ec6f6c6ceac57ab63efb6b46e17ab08d21f69f3a48b38e1534f17
RUN set -eux; \
    arch="$(uname -m)"; \
    case "$arch" in \
        x86_64)  sha="$ZIG_SHA256_X86_64" ;; \
        aarch64) sha="$ZIG_SHA256_AARCH64" ;; \
        *) echo "no zig binary release for $arch" >&2; exit 1 ;; \
    esac; \
    tarball="zig-$arch-linux-$ZIG_VERSION.tar.xz"; \
    curl -fsSL -o "/tmp/$tarball" \
        "https://ziglang.org/download/$ZIG_VERSION/$tarball"; \
    echo "$sha  /tmp/$tarball" | sha256sum -c -; \
    mkdir -p /opt/zig; \
    tar -xJf "/tmp/$tarball" -C /opt/zig --strip-components=1; \
    rm -f "/tmp/$tarball"; \
    ln -s /opt/zig/zig /usr/local/bin/zig; \
    zig version

# Julia, from the upstream generic linux tarballs; ubuntu dropped its julia
# package in 26.04. Shasums are those published in
# julialang-s3.julialang.org/bin/checksums/julia-JULIA_VERSION.sha256, so
# bumping the version means bumping these too.
ARG JULIA_VERSION
ARG JULIA_SHA256_X86_64=4e7e9e776634d24835250de67cde39b0d4af15bc432eb20697e6be6c28ea69e8
ARG JULIA_SHA256_AARCH64=9243c0b524c7f300883240a1ee5ea3916a30e070bff718acf8ccaee31a731ef2
RUN set -eux; \
    arch="$(uname -m)"; \
    case "$arch" in \
        x86_64)  sha="$JULIA_SHA256_X86_64";  dir=x64 ;; \
        aarch64) sha="$JULIA_SHA256_AARCH64"; dir=aarch64 ;; \
        *) echo "no julia binary release for $arch" >&2; exit 1 ;; \
    esac; \
    tarball="julia-$JULIA_VERSION-linux-$arch.tar.gz"; \
    series="$(echo "$JULIA_VERSION" | sed 's/\.[0-9]*$//')"; \
    curl -fsSL -o "/tmp/$tarball" \
        "https://julialang-s3.julialang.org/bin/linux/$dir/$series/$tarball"; \
    echo "$sha  /tmp/$tarball" | sha256sum -c -; \
    mkdir -p /opt/julia; \
    tar -xzf "/tmp/$tarball" -C /opt/julia --strip-components=1; \
    rm -f "/tmp/$tarball"; \
    ln -s /opt/julia/bin/julia /usr/local/bin/julia; \
    julia --version

# What the crack build below needs: patch applies crack's python3 fixups to the
# LLVM source, pkg-config is how crack's configure probes for pcre2, and
# libtirpc carries the XDR functions its runtime links. Installed on every
# architecture, although only x86-64 runs the build, because they are small and
# the list is meant to read as what the toolchain needs.
RUN set -eux; \
    apt-get update; \
    apt-get install -y --no-install-recommends \
        patch pkg-config libpcre2-dev libtirpc-dev; \
    rm -rf /var/lib/apt/lists/*

# Crack, the ragel test suite's -K host language, on x86-64 only.
#
# Crack 1.7 (July 2026) ended a long release gap but did not move off LLVM 3.3
# from 2013: the executor asks llvm-config for the `jit` component, which LLVM
# removed after 3.5. So this builds that LLVM first. Restricted to the X86
# target, with no clang and no bindings, it is a small build by modern
# standards: both builds together took about five minutes on a 6-vCPU box.
#
# Two fixes carry 2013 sources onto a 2026 toolchain. crack publishes
# update_llvm_python.patch for LLVM's python2 build scripts, in its git
# repository rather than in the release tarball, which is why it is fetched on
# its own. The other is the bool typedef in X86DisassemblerDecoder.c, because
# gcc 15 defaults to C23, where bool is a keyword.
#
# Crack installs at /usr/local rather than under /opt like zig and julia above:
# its native builder passes -lCrackNativeRuntime with no -L, so the library has
# to sit on the linker's default search path. LLVM is a build dependency only,
# because crack links its libraries statically: the tree is deleted once crack
# is built, and the smoke test below runs against an image that no longer has
# it. The executor and crackc -B llvm-native were both checked that way. The
# loose end is the libtool .la files crack installs, which still name the LLVM
# libdir; that matters only to someone relinking crack's own libraries in
# here. This is the one place the image differs from the ringleader.yaml
# beside it, which keeps the tree on a box that has room for it.
#
# The shasums are of the artifacts fetched on 2026-09-22. Neither project
# publishes a checksum manifest, so these pin what was downloaded rather than
# an upstream claim, and bumping either version means replacing its shasum too.
#
# Ringleader marks this script non-fatal, on the grounds that a break in a
# dependency this old should cost a box its crack tests rather than its whole
# converge. An image has no such halfway state, so a break fails the build. No
# workflow builds this image, so that costs a developer a local docker build,
# which beats shipping an image that silently has no crack in it.
ARG CRACK_VERSION
ARG LLVM_VERSION
ARG CRACK_SHA256=15c2d64c99564c1d05ebd6a9885e51b4552a449aa9eddc473510a917227509ed
ARG LLVM_SHA256=68766b1e70d05a25e2f502e997a3cb3937187a3296595cf6e0977d5cd6727578
RUN set -eux; \
    if [ "$(uname -m)" != x86_64 ]; then \
        echo "crack: skipped on $(uname -m); the LLVM build here targets x86-64"; \
        exit 0; \
    fi; \
    work="$(mktemp -d /tmp/crack-build-XXXXXX)"; \
    cd "$work"; \
    curl -fsSL -o llvm.tar.gz \
        "https://releases.llvm.org/$LLVM_VERSION/llvm-$LLVM_VERSION.src.tar.gz"; \
    echo "$LLVM_SHA256  llvm.tar.gz" | sha256sum -c -; \
    curl -fsSL -o crack.tar.gz \
        "https://crack-lang.org/downloads/crack-$CRACK_VERSION.tar.gz"; \
    echo "$CRACK_SHA256  crack.tar.gz" | sha256sum -c -; \
    tag="https://raw.githubusercontent.com/crack-lang/crack/rel-$CRACK_VERSION"; \
    curl -fsSL -o llvm-python.patch "$tag/update_llvm_python.patch"; \
    tar -xzf llvm.tar.gz; \
    tar -xzf crack.tar.gz; \
    patch -p0 < llvm-python.patch; \
    guard='#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L'; \
    sed -i "s@^typedef int8_t bool;\$@$guard\ntypedef int8_t bool;\n#endif@" \
        "llvm-$LLVM_VERSION.src/lib/Target/X86/Disassembler/X86DisassemblerDecoder.c"; \
    mkdir llvm-build; \
    cd llvm-build; \
    "../llvm-$LLVM_VERSION.src/configure" --prefix="/opt/llvm-$LLVM_VERSION" \
        --enable-optimized --disable-assertions --enable-targets=x86; \
    make REQUIRES_RTTI=1 BINDINGS_TO_BUILD= -j"$(nproc)"; \
    make install OCAMLDOC= BINDINGS_TO_BUILD=; \
    cd "$work/crack-$CRACK_VERSION"; \
    PATH="/opt/llvm-$LLVM_VERSION/bin:$PATH" ./configure --prefix=/usr/local; \
    make -j"$(nproc)"; \
    make install; \
    ldconfig; \
    cd /; \
    rm -rf "$work" "/opt/llvm-$LLVM_VERSION"; \
    printf 'import crack.io cout;\ncout `crack ok\\n`;\n' > /tmp/smoke.crk; \
    crack /tmp/smoke.crk; \
    rm -f /tmp/smoke.crk
