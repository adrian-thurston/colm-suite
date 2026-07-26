# Packaging and Development Image

`Dockerfile` builds the development environment for this branch: enough to
compile the tree, regenerate the parsers, build the guide, run the test suite
and roll a release. `release.sh` produces the publishable artifacts. The
release process itself is documented in `../CLAUDE.md`.

## Building the image

```
docker build -t ragel6-dev package/
```

Build args:

| Arg | Default | Notes |
| --- | --- | --- |
| `UBUNTU_TAG` | `16.04` | Last release predating ragel 6.10 (Mar 2017). 18.04–22.04 also work. |
| `BUILD_PLATFORM` | `linux/amd64` | ragel and kelbt are x86-era C++ that only compile where `char` is signed, so they fail on arm64. |
| `KELBT_VERSION` | `0.16` | Fetched from colm.net; the only build dependency not in apt. |

Because 16.04 is EOL, the base stage rewrites its apt sources to
`old-releases.ubuntu.com` when the normal mirrors fail.

## What the image contains

- **Build**: `build-essential`, `autoconf`, `automake`, a bootstrap `ragel`
  from apt, and `kelbt` built from source. Together these regenerate the
  parsers, which are baked into the release tarball so end users need only a
  C++ compiler.
- **Guide**: `texlive-latex-{base,recommended,extra}`, `ghostscript`, and
  `fig2dev` (`transfig` on bases before 20.04).
- **Test host languages**: `gobjc`, `golang-go`, `default-jdk`, `ruby`,
  `mono-devel`, `gdc`, plus `txl` when a tarball is supplied.

`runtests` treats a missing compiler as "not interested in this language" and
skips silently, so a gap in the image costs coverage without failing anything.

For C#, `configure` looks for the compiler under its old name
(`AC_CHECK_PROG(GMCS, gmcs, gmcs)`), which newer mono dropped in favour of
`mcs`. The image adds a `gmcs` symlink when mono did not install a real one.
The name matters twice over: `runtests` also keys on it to decide whether to
prefix the binary with the `mono` launcher.

## txl

The `@LANG:indep` test cases are language-neutral. `runtests` translates each
into every host language via `langtrans_<lang>.sh`, which shells out to `txl`;
without it, `runtests` skips all of them for **every** language.

txl is not packaged for apt. Drop a tarball into the build context and it is
picked up automatically:

```
cp ~/installs/txl/txl10.5h.linux64.tar.gz package/
docker build -t ragel6-dev package/
```

- Anything matching `txl*.tar.gz` is detected. Exactly one may be present;
  more than that is an error rather than a guess.
- It must match the build architecture. The plain `linux` tarballs are i386,
  the `linux64` ones x86_64. This is checked against the ELF header rather
  than by running the binary, because under buildx a foreign-arch binary may
  execute happily via qemu and only reveal itself later.
- No install step is needed: txl finds its `lib/` relative to the binary, so
  the image just puts `/opt/txl/bin` on `PATH`.

A `.gitignore` here keeps the tarball out of the repository. That is a
preference rather than a licence requirement — see below — so committing it
instead is a reasonable choice if you would rather the image build with no
setup at all.

### Licence

From `COPYRIGHT.txt` in the distribution: TXL is free of charge for use by
individuals, companies and institutions, and may be copied and redistributed
provided the distribution is kept entire and unmodified and no charge of any
kind is made for it. Published work deriving from or depending upon its use is
asked to acknowledge TXL.

## Using the image

With the tree mounted:

```
./autogen.sh          # only after a base image change; see below
./configure && make
cd test && ./runtests
```

`configure` must be re-run after building a new image. The compiler names are
substituted into `test/runtests` from `runtests.in` at configure time, so an
already-configured tree will not see a newly added language. Check with:

```
grep -e txl_engine -e d_compiler test/runtests
```

`autogen.sh` is needed after changing `UBUNTU_TAG`: it leaves `install-sh`,
`missing` and `depcomp` as symlinks into a version-specific
`/usr/share/automake-N.NN/`, which dangle once the base image moves to a
different automake. The symptom is `configure: error: cannot find install-sh`.

`runtests` stops at the first failure, so pass the language flags for the set
you want to exercise (`-C -D -J -R -A -Z`) rather than assuming all of them
currently pass.

## Releases

`release.sh` builds a throwaway clone of the committed tree, so the working
directory is untouched and the tarball is reproducible from VCS. It collects
`ragel-<ver>.tar.gz`, the versioned guide PDF and `SHA256SUMS` into `dist/`.
It deliberately does not run the tests. See `../CLAUDE.md` for the full
release procedure.
