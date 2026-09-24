/* Terminal styling with xterm 256-colour SGR sequences. */

#ifndef SIMPLE_MD_TERM_STYLE_H
#define SIMPLE_MD_TERM_STYLE_H

#include <stddef.h>
#include <stdio.h>

#define TERM_COLOR_NONE (-1)

#define TERM_ATTR_BOLD      (1u << 0)
#define TERM_ATTR_DIM       (1u << 1)
#define TERM_ATTR_ITALIC    (1u << 2)
#define TERM_ATTR_UNDERLINE (1u << 3)
#define TERM_ATTR_BLINK     (1u << 4)
#define TERM_ATTR_INVERSE   (1u << 5)
#define TERM_ATTR_STRIKE    (1u << 6)

typedef struct {
    int fg;
    int bg;
    unsigned attrs;
} term_style_t;

#define TERM_STYLE_INIT { TERM_COLOR_NONE, TERM_COLOR_NONE, 0 }
#define TERM_STYLE_BUFSIZE 64

int term_style_start(const term_style_t *style, char *buf, size_t bufsize);
int term_style_reset(char *buf, size_t bufsize);
void term_style_fstart(const term_style_t *style, FILE *out);
void term_style_freset(FILE *out);

#endif
