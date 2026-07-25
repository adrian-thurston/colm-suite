#!/bin/bash
#
# Roll a Ragel 6.x release: regenerate the parsers, build the guide, and
# produce the publishable artifacts.
#
#   ragel-<ver>.tar.gz     source dist; bundles the PDF and pre-generated
#                          parser sources (end users need neither kelbt nor
#                          pdflatex)
#   ragel-guide-<ver>.pdf  the guide
#   SHA256SUMS
#
# Requires a bootstrap ragel, kelbt, autoconf/automake, pdflatex and
# fig2dev/transfig; package/Dockerfile builds an image with all of it.
#
# The build happens in a throwaway clone, never in the source tree, so the
# tarball is reproducible from VCS and the working directory is left alone.
# That means uncommitted edits are NOT part of the release: bump the version
# in configure.ac, commit, and tag ragel-<ver> before running this.
#
# The test suite is deliberately not run here; run it separately:
#
#   cd test && ./runtests -C -Z
#
# (-C covers C/C++ and obj-C, -Z is Go, over the full -T0..-G2 x -n/-m/-l/-e
# matrix. D is excluded: gdc 8.x rejects the 2009-era D test sources and
# codegen, which is pre-existing language drift.)
#
# Afterwards, sign and publish on the host:
#
#   gpg --armor --detach-sign dist/ragel-<ver>.tar.gz
#
# then upload the .tar.gz, .tar.gz.asc and ragel-guide-<ver>.pdf to
# colm.net/files/ragel/ and update the download page.

set -eu

usage()
{
	cat <<-EOF
	usage: $(basename "$0") [-s SRCDIR] [-o OUTDIR] [-r REF] [-k]

	  -s SRCDIR  repository to build from    (default: the tree containing this script)
	  -o OUTDIR  where to write artifacts    (default: SRCDIR/dist)
	  -r REF     commit/tag/branch to build  (default: HEAD)
	  -k         keep the build directory instead of deleting it
	EOF
}

srcdir=
outdir=
ref=HEAD
keep=false

while getopts "s:o:r:kh" opt; do
	case $opt in
		s) srcdir="$OPTARG";;
		o) outdir="$OPTARG";;
		r) ref="$OPTARG";;
		k) keep=true;;
		h) usage; exit 0;;
		*) usage >&2; exit 1;;
	esac
done

# A tree bind-mounted into a container is owned by the host user, which git
# refuses to touch as root. Scope the exemption to this script rather than
# editing the developer's global config.
git()
{
	command git -c safe.directory='*' "$@"
}

if [ -z "$srcdir" ]; then
	srcdir="$( cd "$( dirname "$0" )/.." && pwd )"
fi
srcdir="$( cd "$srcdir" && git rev-parse --show-toplevel )"
[ -n "$outdir" ] || outdir="$srcdir/dist"
# Absolute, because the build runs from elsewhere.
mkdir -p "$outdir"
outdir="$( cd "$outdir" && pwd )"

# The clone carries committed state only; say so if that differs from what the
# developer is looking at.
if [ -n "$( git -C "$srcdir" status --porcelain )" ]; then
	echo "warning: $srcdir has uncommitted changes; building $ref without them" >&2
fi

builddir="$( mktemp -d "${TMPDIR:-/tmp}/ragel-release.XXXXXX" )"
$keep || trap 'rm -rf "$builddir"' EXIT

echo "==> cloning $srcdir ($ref) into $builddir"
git clone --quiet --shared "$srcdir" "$builddir/src"
git -C "$builddir/src" checkout --quiet --detach "$ref"

cd "$builddir/src"

echo "==> regenerating the build system"
./autogen.sh
./configure

# Builds the ragel binary, regenerates the parsers with kelbt + the bootstrap
# ragel, and produces doc/ragel-guide.pdf.
echo "==> building"
make

echo "==> rolling the tarball"
make dist

tarball="$( ls ragel-*.tar.gz )"
ver="$( echo "$tarball" | sed -n 's/^ragel-\(.*\)\.tar\.gz$/\1/p' )"
[ -n "$ver" ] || { echo "error: cannot parse a version out of '$tarball'" >&2; exit 1; }

cp "$tarball" "$outdir/"
cp doc/ragel-guide.pdf "$outdir/ragel-guide-${ver}.pdf"
( cd "$outdir" && sha256sum "ragel-${ver}.tar.gz" "ragel-guide-${ver}.pdf" > SHA256SUMS )

echo "==> ragel $ver artifacts in $outdir"
ls -l "$outdir"
