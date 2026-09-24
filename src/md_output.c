#include "md_output.h"
#include "sm_assert.h"

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
