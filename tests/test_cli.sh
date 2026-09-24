#!/bin/sh
set -eu

fail() {
    echo "FAIL: $*" >&2
    exit 1
}

tmp_dir=$(mktemp -d "${TMPDIR:-/tmp}/simple-md-cli.XXXXXX")
trap 'rm -rf "$tmp_dir"' EXIT HUP INT TERM

./simple-md --help >"$tmp_dir/help"
grep -Fq 'Usage: simple-md' "$tmp_dir/help" || fail "--help omitted usage"

./simple-md --version >"$tmp_dir/version"
grep -Eq '^simple-md [0-9]+\.[0-9]+\.[0-9]+$' "$tmp_dir/version" ||
    fail "--version has an unexpected format"

if ./simple-md --width=0 </dev/null >"$tmp_dir/out" 2>"$tmp_dir/err"; then
    fail "--width=0 unexpectedly succeeded"
fi
grep -Fq 'invalid width' "$tmp_dir/err" || fail "invalid width was not explained"

if ./simple-md --unknown </dev/null >"$tmp_dir/out" 2>"$tmp_dir/err"; then
    fail "unknown option unexpectedly succeeded"
fi
grep -Fq 'unknown option' "$tmp_dir/err" || fail "unknown option was not explained"

if ./simple-md "$tmp_dir/missing.md" >"$tmp_dir/out" 2>"$tmp_dir/err"; then
    fail "missing file unexpectedly succeeded"
fi
grep -Fq 'cannot open' "$tmp_dir/err" || fail "missing file was not explained"

: >"$tmp_dir/empty.md"
./simple-md "$tmp_dir/empty.md"
./simple-md </dev/null

echo "test_cli: PASS"
