#include "md_search.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void fail(const char *message) {
    fprintf(stderr, "FAIL: %s\n", message);
    exit(1);
}

int main(void) {
    md_span_t spans0[] = {
        { .text = "Alpha ", .width = 6 },
        { .text = "beta", .width = 4 },
    };
    md_span_t spans1[] = {
        { .text = "beta BETA beta", .width = 14 },
    };
    md_span_t spans2[] = {
        { .text = "caf\xc3\xa9", .width = 4 },
    };
    md_display_line_t lines[] = {
        { .spans = spans0, .span_count = 2 },
        { .spans = spans1, .span_count = 1 },
        { .spans = spans2, .span_count = 1 },
    };
    md_layout_t layout = { .lines = lines, .line_count = 3 };
    md_search_t search;
    md_search_init(&search);

    if (!md_search_begin(&search, &layout, "alpha beta", 0))
        fail("search did not match across styled spans");
    const md_match_t *match = md_search_current(&search);
    if (match == NULL || match->line != 0 || match->byte_start != 0 ||
        match->byte_end != 10)
        fail("cross-span match coordinates were wrong");

    if (!md_search_begin(&search, &layout, "BeTa", 1) || search.match_count != 4)
        fail("ASCII case-insensitive search did not find every occurrence");
    match = md_search_current(&search);
    if (match == NULL || match->line != 1 || match->byte_start != 0)
        fail("search did not begin at the requested line");

    match = md_search_next(&search);
    if (match == NULL || match->line != 1 || match->byte_start != 5)
        fail("next search did not advance to the next occurrence");
    md_search_next(&search);
    match = md_search_next(&search);
    if (match == NULL || match->line != 0 || match->byte_start != 6)
        fail("next search did not wrap to the first occurrence");

    match = md_search_previous(&search);
    if (match == NULL || match->line != 1 || match->byte_start != 10)
        fail("previous search did not wrap to the final occurrence");
    match = md_search_next(&search);
    if (match == NULL || match->line != 0 || match->byte_start != 6)
        fail("next then previous were not symmetric");

    if (!md_search_begin(&search, &layout, "ALPHA", 2))
        fail("search did not wrap to the beginning");
    match = md_search_current(&search);
    if (match == NULL || match->line != 0)
        fail("wrapped search selected the wrong line");

    if (!md_search_begin(&search, &layout, "caf\xc3\xa9", 0))
        fail("exact UTF-8 search failed");
    if (md_search_begin(&search, &layout, "", 0))
        fail("empty search unexpectedly matched");
    if (md_search_begin(&search, &layout, "missing", 0))
        fail("missing query unexpectedly matched");
    if (md_search_next(&search) != NULL)
        fail("next search matched without an active result set");
    if (md_search_previous(&search) != NULL)
        fail("previous search matched without an active result set");

    md_search_destroy(&search);
    puts("test_md_search: PASS");
    return 0;
}
