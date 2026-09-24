#include "md_output.h"
#include "sm_assert.h"
#include "term_style.h"
#include "term_link.h"

#include <stddef.h>

int md_output_write_plain(FILE *out, const md_layout_t *layout) {
    ASSERT_MSG(out != NULL, "md_output_write_plain: out is NULL");
    ASSERT_MSG(layout != NULL, "md_output_write_plain: layout is NULL");

    for (int line_index = 0; line_index < layout->line_count; line_index++) {
        const md_display_line_t *line = &layout->lines[line_index];
        for (int span_index = 0; span_index < line->span_count; span_index++) {
            if (fputs(line->spans[span_index].text, out) == EOF) return -1;
        }
        if (fputc('\n', out) == EOF) return -1;
    }

    return ferror(out) ? -1 : 0;
}

int md_output_write_styled(FILE *out, const md_layout_t *layout) {
    ASSERT_MSG(out != NULL, "md_output_write_styled: out is NULL");
    ASSERT_MSG(layout != NULL, "md_output_write_styled: layout is NULL");

    for (int line_index = 0; line_index < layout->line_count; line_index++) {
        const md_display_line_t *line = &layout->lines[line_index];
        for (int span_index = 0; span_index < line->span_count; span_index++) {
            const md_span_t *span = &line->spans[span_index];
            int linked = term_link_fstart(span->link_url, out);
            term_style_fstart(&span->style, out);
            if (fputs(span->text, out) == EOF) return -1;
            term_style_freset(out);
            if (linked) term_link_fend(out);
        }
        if (fputc('\n', out) == EOF) return -1;
    }

    return ferror(out) ? -1 : 0;
}
