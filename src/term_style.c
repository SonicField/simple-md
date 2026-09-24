/* Terminal styling with xterm 256-colour SGR sequences. */

#include "term_style.h"
#include "sm_assert.h"

#include <stdio.h>
#include <string.h>

int term_style_start(const term_style_t *style, char *buf, size_t bufsize) {
    ASSERT_MSG(style != NULL, "term_style_start: style is NULL");
    ASSERT_MSG(buf != NULL, "term_style_start: buf is NULL");
    if (bufsize < 5) return -1;

    if (style->attrs == 0 && style->fg == TERM_COLOR_NONE &&
        style->bg == TERM_COLOR_NONE) {
        buf[0] = '\0';
        return 0;
    }

    char params[48];
    int offset = 0;
    static const struct { unsigned mask; int code; } attributes[] = {
        { TERM_ATTR_BOLD,      1 },
        { TERM_ATTR_DIM,       2 },
        { TERM_ATTR_ITALIC,    3 },
        { TERM_ATTR_UNDERLINE, 4 },
        { TERM_ATTR_BLINK,     5 },
        { TERM_ATTR_INVERSE,   7 },
        { TERM_ATTR_STRIKE,    9 },
    };

    for (size_t i = 0; i < sizeof(attributes) / sizeof(attributes[0]); i++) {
        if (style->attrs & attributes[i].mask) {
            if (offset > 0) params[offset++] = ';';
            offset += snprintf(params + offset, sizeof(params) - (size_t)offset,
                               "%d", attributes[i].code);
        }
    }

    if (style->fg >= 0 && style->fg <= 255) {
        if (offset > 0) params[offset++] = ';';
        offset += snprintf(params + offset, sizeof(params) - (size_t)offset,
                           "38;5;%d", style->fg);
    }
    if (style->bg >= 0 && style->bg <= 255) {
        if (offset > 0) params[offset++] = ';';
        offset += snprintf(params + offset, sizeof(params) - (size_t)offset,
                           "48;5;%d", style->bg);
    }

    int needed = 2 + offset + 1 + 1;
    if ((size_t)needed > bufsize) return -1;

    int written = snprintf(buf, bufsize, "\033[%sm", params);
    if (written < 0 || (size_t)written >= bufsize) return -1;
    return written;
}

int term_style_reset(char *buf, size_t bufsize) {
    ASSERT_MSG(buf != NULL, "term_style_reset: buf is NULL");
    if (bufsize < 5) return -1;
    memcpy(buf, "\033[0m", 5);
    return 4;
}

void term_style_fstart(const term_style_t *style, FILE *out) {
    ASSERT_MSG(style != NULL, "term_style_fstart: style is NULL");
    ASSERT_MSG(out != NULL, "term_style_fstart: out is NULL");

    char buf[TERM_STYLE_BUFSIZE];
    int written = term_style_start(style, buf, sizeof(buf));
    if (written > 0) fwrite(buf, 1, (size_t)written, out);
}

void term_style_freset(FILE *out) {
    ASSERT_MSG(out != NULL, "term_style_freset: out is NULL");
    fputs("\033[0m", out);
}
