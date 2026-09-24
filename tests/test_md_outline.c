#include "md_outline.h"
#include "md_parse.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void fail(const char *message) {
    fprintf(stderr, "FAIL: %s\n", message);
    exit(1);
}

int main(void) {
    const char *markdown =
        "Opening paragraph.\n\n"
        "# First **heading**\n\n"
        "Text.\n\n"
        "### Caf\xc3\xa9 `code`\n\n"
        "# First heading\n";
    md_block_node_t *document = md_parse(markdown);
    md_layout_t *layout = md_render(document, 60);
    md_outline_t outline;
    md_outline_init(&outline);
    md_outline_build(&outline, document, layout);

    if (outline.count != 3) fail("outline did not preserve repeated headings");
    if (outline.entries[0].level != 1 ||
        strcmp(outline.entries[0].title, "First heading") != 0)
        fail("formatted heading was not flattened correctly");
    if (outline.entries[1].level != 3 ||
        strcmp(outline.entries[1].title, "Caf\xc3\xa9 code") != 0)
        fail("Unicode/code heading was not flattened correctly");
    if (outline.entries[0].line < 0 ||
        layout->lines[outline.entries[0].line].span_count == 0)
        fail("outline destination points to a blank line");
    if (outline.entries[2].line <= outline.entries[1].line)
        fail("outline destinations are not in document order");

    md_outline_destroy(&outline);
    md_layout_destroy(layout);
    md_block_destroy(document);

    document = md_parse("No headings here.\n");
    layout = md_render(document, 60);
    md_outline_init(&outline);
    md_outline_build(&outline, document, layout);
    if (outline.count != 0) fail("heading-free document produced outline entries");
    md_outline_destroy(&outline);
    md_layout_destroy(layout);
    md_block_destroy(document);

    puts("test_md_outline: PASS");
    return 0;
}
