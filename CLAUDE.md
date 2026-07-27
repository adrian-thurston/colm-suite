# CLAUDE.md

Guidance for Claude Code (claude.ai/code) when working in this repository — the
standalone Ragel 6.x maintenance branch.

## Making a Release

Releases are published under `colm.net/files/ragel/` (linked from
`colm.net/open-source/ragel`): `ragel-<ver>.tar.gz`, its detached signature
`.asc`, and `ragel-guide-<ver>.pdf`.

The version comes from `configure.ac` — `AC_INIT(ragel, <ver>)` and
`PUBDATE="<Month Year>"` — and feeds the `--version` banner and the guide title
page. Release tags are lightweight, named `ragel-<ver>`.

Build dependencies:

- C/C++ compiler, `autoconf`, `automake` (run `./autogen.sh`; no libtool).
- Parser regeneration: a bootstrap `ragel` (any 6.x, e.g. from apt) and `kelbt`
  — the only dependency not in apt, built from source from
  `colm.net/files/kelbt/`.
- Manual: `pdflatex` (`texlive-latex-base`, `-recommended`, `-extra`),
  `fig2dev`/`transfig`, and `ghostscript`.
- Tests: a compiler for each host language — `gobjc`, `gdc`, `default-jdk`,
  `ruby`, `mono-devel`, `golang-go` — plus `txl` for the `@LANG:indep` cases,
  which are translated into every language. The suite treats a missing
  compiler as "not interested in this language" and skips silently, so an
  incomplete environment costs coverage without failing anything.
- `gpg` for signing.

`package/Dockerfile` builds an image with all of the above installed — the
recommended environment for both development and release builds. See
`package/README.md`; txl is the one piece it cannot install unaided, and needs
a tarball dropped into the build context. Build on **x86_64**: `ragel`/`kelbt`
are x86-era C++ that fail to compile where `char` is unsigned (arm64); the
tarball and PDF are architecture-neutral regardless.

`./autogen.sh && ./configure && make && make dist` produces the tarball, with
the parser sources and guide PDF baked in so end users need only a C++ compiler.
`package/release.sh` wraps that up: it builds a throwaway clone of the committed
tree (so the working directory is untouched and the tarball is reproducible from
VCS) and collects the tarball, the versioned guide PDF and `SHA256SUMS` into
`dist/`. It does not run the tests — do that separately with
`cd test && ./runtests`, which covers every host language over the full
`-T0`…`-G2` × `-n`/`-m`/`-l`/`-e` matrix. To cut a release:

1. Bump the version and `PUBDATE` in `configure.ac`; add a `ChangeLog` entry.
2. Commit, then tag `ragel-<ver>`.
3. Run the test suite, then `package/release.sh` to get the tarball + guide PDF.
4. Sign: `gpg --armor --detach-sign ragel-<ver>.tar.gz`.
5. Upload the `.tar.gz`, `.asc`, and `ragel-guide-<ver>.pdf` to
   `colm.net/files/ragel/` and update the download page.

## Commit Messages

Prefix the subject with a Conventional Commits type:

- `fix:` — bug fix (e.g. `fix: use-after-free in pruneExpansions`)
- `feat:` — new functionality
- `chore:` — build, tooling, or housekeeping (e.g. `chore: add multi-stage Dockerfile to build a Ragel 6.x release`)
- `docs:`, `refactor:`, `test:` — as appropriate

Keep the subject short and lowercase after the colon. For non-trivial changes,
add a blank line and a body wrapped at ~75 columns explaining the *why*, and
reference the introducing commit or issue where relevant.
