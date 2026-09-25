# Changelog

## Unreleased

- Harden required parser and renderer allocations and make GCC static analysis
  a CI release gate.
- Add lightweight Java and Rust syntax highlighting through the existing
  C-family scanner.
- Add one shared syntax highlighter for POSIX-style sh, Bash, Zsh, and Ksh
  code fences.

## 0.2.0 — 2026-09-24

- Render plain text without control sequences when output is redirected.
- Select full-screen paging automatically, with explicit `always` and `never`
  overrides.
- Honor `--no-color` and the `NO_COLOR` environment variable.
- Search rendered text with `/`, repeat forward with `n`, and backward with
  `N`; results wrap and the current match is highlighted.
- Browse and jump through document headings with `o`.
- Emit sanitized OSC 8 hyperlinks on interactive terminals, with explicit
  link-mode overrides.

## 0.1.0 — 2026-09-24

- Initial standalone extraction from NBS Framework.
