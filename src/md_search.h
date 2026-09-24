#ifndef MD_SEARCH_H
#define MD_SEARCH_H

#include "md_render.h"

#include <stddef.h>

#define MD_SEARCH_QUERY_MAX 255

typedef struct {
    int line;
    size_t byte_start;
    size_t byte_end;
} md_match_t;

typedef struct {
    char query[MD_SEARCH_QUERY_MAX + 1];
    md_match_t *matches;
    int match_count;
    int current_index;
} md_search_t;

void md_search_init(md_search_t *search);
void md_search_destroy(md_search_t *search);

/* Build a match index and select the first match at or after start_line,
 * wrapping to the beginning if needed. Returns one when a match exists. */
int md_search_begin(md_search_t *search, const md_layout_t *layout,
                    const char *query, int start_line);

const md_match_t *md_search_current(const md_search_t *search);
const md_match_t *md_search_next(md_search_t *search);

#endif
