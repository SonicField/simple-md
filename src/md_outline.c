#include "md_outline.h"
#include "sm_alloc.h"
#include "sm_assert.h"

#include <stdlib.h>
#include <string.h>

static void append_text(char **buffer, size_t *length, const char *text) {
    size_t addition = strlen(text);
    char *grown = sm_realloc(*buffer, *length + addition + 1);
    *buffer = grown;
    memcpy(*buffer + *length, text, addition);
    *length += addition;
    (*buffer)[*length] = '\0';
}

static void append_inlines(char **buffer, size_t *length,
                           const md_inline_node_t *inline_node) {
    for (const md_inline_node_t *node = inline_node; node != NULL; node = node->next) {
        switch (node->type) {
        case MD_INLINE_TEXT:
        case MD_INLINE_CODE:
            if (node->text != NULL) append_text(buffer, length, node->text);
            break;
        case MD_INLINE_SOFTBREAK:
        case MD_INLINE_HARDBREAK:
            append_text(buffer, length, " ");
            break;
        case MD_INLINE_BOLD:
        case MD_INLINE_ITALIC:
        case MD_INLINE_BOLD_ITALIC:
        case MD_INLINE_LINK:
            append_inlines(buffer, length, node->children);
            break;
        }
    }
}

static int heading_line(const md_layout_t *layout, int source_block) {
    for (int i = 0; i < layout->line_count; i++) {
        if (layout->lines[i].source_block == source_block &&
            layout->lines[i].span_count > 0) return i;
    }
    return -1;
}

void md_outline_init(md_outline_t *outline) {
    ASSERT_MSG(outline != NULL, "md_outline_init: outline is NULL");
    memset(outline, 0, sizeof(*outline));
}

void md_outline_destroy(md_outline_t *outline) {
    if (outline == NULL) return;
    for (int i = 0; i < outline->count; i++) free(outline->entries[i].title);
    free(outline->entries);
    md_outline_init(outline);
}

void md_outline_build(md_outline_t *outline, const md_block_node_t *document,
                      const md_layout_t *layout) {
    ASSERT_MSG(outline != NULL, "md_outline_build: outline is NULL");
    ASSERT_MSG(document != NULL, "md_outline_build: document is NULL");
    ASSERT_MSG(layout != NULL, "md_outline_build: layout is NULL");

    md_outline_destroy(outline);
    const md_block_node_t *block = document->type == MD_BLOCK_DOCUMENT
        ? document->children : document;
    int block_id = 0;
    while (block != NULL) {
        if (block->type == MD_BLOCK_HEADING) {
            md_outline_entry_t *grown = sm_realloc(
                outline->entries, (size_t)(outline->count + 1) * sizeof(*grown));
            outline->entries = grown;

            char *title = NULL;
            size_t length = 0;
            append_inlines(&title, &length, block->inlines);
            if (title == NULL) title = sm_strdup("");

            outline->entries[outline->count++] = (md_outline_entry_t) {
                .level = block->level,
                .source_block = block_id,
                .line = heading_line(layout, block_id),
                .title = title,
            };
        }
        block_id++;
        block = block->next;
    }
}
