#!/usr/bin/env python3
"""Run every normative CommonMark 0.31.2 example."""
from pathlib import Path
import difflib
import re
import subprocess
import sys
from commonmark_normalize import normalize_html

ROOT = Path(__file__).resolve().parent.parent
SPEC = ROOT / "tests/commonmark/spec.txt"
RENDERER = ROOT / "build/commonmark-render"

def examples():
    section, state, markdown, html = "", 0, [], []
    number = 0
    fence = "`" * 32
    for line_number, line in enumerate(SPEC.read_text(encoding="utf-8").splitlines(True), 1):
        stripped = line.strip()
        if re.match(r"^#+ ", line) and state == 0:
            section = re.sub(r"^#+ ", "", line).strip()
        elif stripped.startswith(fence + " example"):
            state, markdown, html, start = 1, [], [], line_number
        elif state == 1 and stripped == ".":
            state = 2
        elif state == 2 and stripped == fence:
            number += 1
            yield (number, section, start,
                   "".join(markdown).replace("→", "\t"),
                   "".join(html).replace("→", "\t"))
            state = 0
        elif state == 1:
            markdown.append(line)
        elif state == 2:
            html.append(line)

def main():
    failures = []
    tests = list(examples())
    for number, section, line, markdown, expected in tests:
        process = subprocess.run([str(RENDERER)], input=markdown, text=True,
                                 stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                 check=False)
        if (process.returncode or
                normalize_html(process.stdout) != normalize_html(expected)):
            diff = "".join(difflib.unified_diff(
                expected.splitlines(True), process.stdout.splitlines(True),
                fromfile="expected", tofile="actual"))
            failures.append((number, section, line, process.returncode,
                             process.stderr, markdown, diff))
    for number, section, line, code, stderr, markdown, diff in failures[:10]:
        print(f"Example {number}, {section}, spec line {line}, exit {code}")
        print(repr(markdown))
        if stderr: print(stderr)
        print(diff)
    passed = len(tests) - len(failures)
    print(f"{passed} passed, {len(failures)} failed, {len(tests)} total (CommonMark 0.31.2)")
    return bool(failures)

if __name__ == "__main__":
    sys.exit(main())
