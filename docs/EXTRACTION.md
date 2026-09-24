# Extraction record

## Goal

Create a standalone, reusable terminal Markdown viewer named `simple-md`
without changing the source NBS Framework repository.

## Provenance

- Source repository: `https://github.com/SonicField/nbs-framework.git`
- Source snapshot: `a1084a3aaab1d933585438b2061ec8c86150f1c2`
- Original viewer: `src/nbs-md-viewer`
- Imported support code: terminal styles, Unicode display widths, Unicode BiDi,
  and the always-on assertion macro
- License: MIT, retained unchanged

The repository was created by path-limiting `git fast-export` from the source
snapshot and importing that stream into a fresh repository. This retained 14
relevant commits while excluding unrelated NBS files.

## Extraction decisions

- Viewer sources live in `src/`; unit tests live in `tests/`.
- Bundled support modules received project-neutral filenames and symbols.
- NBS-only handle-colour APIs were removed from the terminal-style module
  because the viewer never calls them.
- The executable accepts either a filename or standard input.
- Both `q` and bare Escape quit. The imported implementation accepted Escape,
  while older documentation and one test still claimed `q` worked.
- NBS-dependent shell tests were replaced with a real pseudo-terminal test
  using only Python's standard library.

## Falsification criteria

The extraction is not complete if any of these checks fails:

1. A tracked source or build file still depends on an NBS path or symbol.
2. A clean checkout cannot build with the documented `make` command.
3. Any of the 214 imported unit cases regresses.
4. The executable cannot render the fixture and exit via both documented keys.
5. Alternate-screen entry is not paired with restoration on normal exit.
6. ASan or UBSan reports a failure while running the unit suite.
7. Installation ignores `PREFIX` or `DESTDIR`.

Run `make test` and `make sanitize` to reproduce the behavioral checks. A
clean-room copy build additionally detects accidental dependencies on files
outside this repository.

## Verification record

On 2026-09-24:

- The unmodified extracted source passed all 214 imported unit cases.
- The standalone layout passed the same 214 cases with GCC and strict warnings.
- ASan and UBSan reported no failures across the unit suite.
- Real-PTY tests rendered a heading and Unicode table, accepted both a file and
  piped input, exited through both `q` and Escape, and emitted matching
  alternate-screen entry and restoration sequences.
- A staged install placed an executable at `DESTDIR/usr/bin/simple-md`.
- A copy under `/tmp`, outside both source repositories, rebuilt and passed the
  complete suite.
- No NBS path or symbol remained in source, tests, build files, or CI.

Clang was unavailable in the extraction environment, so the configured Clang
CI job was not run locally. An optional Valgrind run was inconclusive because
the local wrapper stalled on the first test; ASan/UBSan is the completed
dynamic-analysis result.
