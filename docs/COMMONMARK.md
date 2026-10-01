# CommonMark compliance

We believe `simple-md` is compliant with CommonMark 0.31.2.

This is an evidence-based belief, not a proof that every possible Markdown
document is handled correctly. The current implementation passes all 652
normative examples from the official CommonMark 0.31.2 specification. Run the
same check locally with:

```sh
make commonmark
```

The conformance test renders each normative Markdown example with extensions
disabled, normalizes insignificant HTML serialization differences using the
CommonMark test-suite rules, and compares the result with the specified HTML.
The corpus is stored in [`tests/commonmark/spec.txt`](../tests/commonmark/spec.txt)
so the result does not depend on network availability.

The terminal reader uses the same parser through `simple-md`'s AST adapter.
Focused adapter tests cover headings, indented code blocks, reference links,
images, raw HTML, entities, lists, emphasis, links, tables, and line breaks.

## Scope and limitations

- Compliance refers to CommonMark 0.31.2 parsing semantics.
- GFM pipe tables are enabled as an additional extension in the terminal reader.
- Raw HTML is displayed as source because this is a terminal reader, not a web
  browser.
- Images are represented by linked alternative text because the terminal
  renderer does not display bitmap content.
- A newly discovered counterexample would falsify this compliance belief and
  should be added as a regression test before its fix.

Passing the corpus means the implementation resisted the official conformance
check. It does not establish mathematical proof of correctness.
