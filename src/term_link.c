#include "term_link.h"
#include "sm_assert.h"

#include <string.h>

static int hyperlinks_enabled = 1;

void term_link_set_enabled(int enabled) {
    hyperlinks_enabled = enabled != 0;
}

int term_link_is_enabled(void) {
    return hyperlinks_enabled;
}

int term_link_url_is_safe(const char *url) {
    if (url == NULL || url[0] == '\0') return 0;
    for (const unsigned char *p = (const unsigned char *)url; *p != '\0'; p++) {
        if (*p < 0x20 || *p == 0x7f) return 0;
    }
    return 1;
}

int term_link_fstart(const char *url, FILE *out) {
    ASSERT_MSG(out != NULL, "term_link_fstart: out is NULL");
    if (!hyperlinks_enabled || !term_link_url_is_safe(url)) return 0;
    fputs("\033]8;;", out);
    fputs(url, out);
    fputs("\033\\", out);
    return 1;
}

void term_link_fend(FILE *out) {
    ASSERT_MSG(out != NULL, "term_link_fend: out is NULL");
    if (hyperlinks_enabled) fputs("\033]8;;\033\\", out);
}
