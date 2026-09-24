#include "md_search.h"
#include "sm_assert.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char fold_ascii(unsigned char byte) {
    return byte < 0x80 ? (unsigned char)tolower(byte) : byte;
}

static int bytes_equal_folded(const char *text, const char *query, size_t length) {
    for (size_t i = 0; i < length; i++) {
        if (fold_ascii((unsigned char)text[i]) !=
            fold_ascii((unsigned char)query[i])) return 0;
    }
    return 1;
}

static char *line_text(const md_display_line_t *line, size_t *length) {
    size_t total = 0;
    for (int i = 0; i < line->span_count; i++) total += strlen(line->spans[i].text);

    char *text = malloc(total + 1);
    ASSERT_MSG(text != NULL, "md_search: failed to allocate %zu bytes", total + 1);
    size_t offset = 0;
    for (int i = 0; i < line->span_count; i++) {
        size_t span_length = strlen(line->spans[i].text);
        memcpy(text + offset, line->spans[i].text, span_length);
        offset += span_length;
    }
    text[offset] = '\0';
    *length = total;
    return text;
}

static void add_match(md_search_t *search, int line, size_t start, size_t end) {
    int count = search->match_count;
    md_match_t *grown = realloc(search->matches,
                                (size_t)(count + 1) * sizeof(*grown));
    ASSERT_MSG(grown != NULL, "md_search: failed to grow match index to %d", count + 1);
    search->matches = grown;
    search->matches[count] = (md_match_t) {
        .line = line,
        .byte_start = start,
        .byte_end = end,
    };
    search->match_count++;
}

void md_search_init(md_search_t *search) {
    ASSERT_MSG(search != NULL, "md_search_init: search is NULL");
    memset(search, 0, sizeof(*search));
    search->current_index = -1;
}

void md_search_destroy(md_search_t *search) {
    if (search == NULL) return;
    free(search->matches);
    md_search_init(search);
}

int md_search_begin(md_search_t *search, const md_layout_t *layout,
                    const char *query, int start_line) {
    ASSERT_MSG(search != NULL, "md_search_begin: search is NULL");
    ASSERT_MSG(layout != NULL, "md_search_begin: layout is NULL");
    ASSERT_MSG(query != NULL, "md_search_begin: query is NULL");

    free(search->matches);
    search->matches = NULL;
    search->match_count = 0;
    search->current_index = -1;
    snprintf(search->query, sizeof(search->query), "%s", query);

    size_t query_length = strlen(search->query);
    if (query_length == 0) return 0;

    for (int line_index = 0; line_index < layout->line_count; line_index++) {
        size_t length = 0;
        char *text = line_text(&layout->lines[line_index], &length);
        if (query_length <= length) {
            for (size_t offset = 0; offset + query_length <= length; offset++) {
                if (bytes_equal_folded(text + offset, search->query, query_length)) {
                    add_match(search, line_index, offset, offset + query_length);
                }
            }
        }
        free(text);
    }

    if (search->match_count == 0) return 0;
    for (int i = 0; i < search->match_count; i++) {
        if (search->matches[i].line >= start_line) {
            search->current_index = i;
            return 1;
        }
    }
    search->current_index = 0;
    return 1;
}

const md_match_t *md_search_current(const md_search_t *search) {
    if (search == NULL || search->current_index < 0 ||
        search->current_index >= search->match_count) return NULL;
    return &search->matches[search->current_index];
}
