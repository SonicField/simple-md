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
grep -Fq -- '--pager=MODE' "$tmp_dir/help" || fail "--help omitted pager modes"
grep -Fq -- '--no-color' "$tmp_dir/help" || fail "--help omitted color control"
grep -Fq -- '--links=MODE' "$tmp_dir/help" || fail "--help omitted link modes"

./simple-md --version >"$tmp_dir/version"
grep -Fxq 'simple-md 0.2.0' "$tmp_dir/version" ||
    fail "--version has an unexpected format"

if ./simple-md --width=0 </dev/null >"$tmp_dir/out" 2>"$tmp_dir/err"; then
    fail "--width=0 unexpectedly succeeded"
fi
grep -Fq 'invalid width' "$tmp_dir/err" || fail "invalid width was not explained"

if ./simple-md --unknown </dev/null >"$tmp_dir/out" 2>"$tmp_dir/err"; then
    fail "unknown option unexpectedly succeeded"
fi
grep -Fq 'unknown option' "$tmp_dir/err" || fail "unknown option was not explained"

if ./simple-md --pager=sometimes </dev/null >"$tmp_dir/out" 2>"$tmp_dir/err"; then
    fail "invalid pager mode unexpectedly succeeded"
fi
grep -Fq 'invalid pager mode' "$tmp_dir/err" ||
    fail "invalid pager mode was not explained"

if ./simple-md --links=sometimes </dev/null >"$tmp_dir/out" 2>"$tmp_dir/err"; then
    fail "invalid link mode unexpectedly succeeded"
fi
grep -Fq 'invalid link mode' "$tmp_dir/err" ||
    fail "invalid link mode was not explained"

if ./simple-md "$tmp_dir/missing.md" >"$tmp_dir/out" 2>"$tmp_dir/err"; then
    fail "missing file unexpectedly succeeded"
fi
grep -Fq 'cannot open' "$tmp_dir/err" || fail "missing file was not explained"

: >"$tmp_dir/empty.md"
./simple-md "$tmp_dir/empty.md"
./simple-md </dev/null

printf '# Heading\n\nA **bold** [link](https://example.com).\n' |
    ./simple-md --width=40 >"$tmp_dir/plain"
grep -Fq 'Heading' "$tmp_dir/plain" || fail "plain output omitted heading text"
grep -Fq 'A bold link.' "$tmp_dir/plain" || fail "plain output omitted rendered body text"
if LC_ALL=C grep -q "$(printf '\033')" "$tmp_dir/plain"; then
    fail "plain output contained an escape byte"
fi
if grep -Fq '**bold**' "$tmp_dir/plain"; then
    fail "plain output leaked Markdown emphasis delimiters"
fi
if LC_ALL=C grep -q "$(printf '\033]8;')" "$tmp_dir/plain"; then
    fail "redirected output contained an OSC 8 hyperlink"
fi

echo "test_cli: PASS"
