#ifndef MD_OUTLINE_H
#define MD_OUTLINE_H

#include "md_ast.h"
#include "md_render.h"

typedef struct {
    int level;
    int source_block;
    int line;
    char *title;
} md_outline_entry_t;

typedef struct {
    md_outline_entry_t *entries;
    int count;
} md_outline_t;

void md_outline_init(md_outline_t *outline);
void md_outline_destroy(md_outline_t *outline);
void md_outline_build(md_outline_t *outline, const md_block_node_t *document,
                      const md_layout_t *layout);

#endif
