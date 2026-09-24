#ifndef SIMPLE_MD_TERM_LINK_H
#define SIMPLE_MD_TERM_LINK_H

#include <stdio.h>

void term_link_set_enabled(int enabled);
int term_link_is_enabled(void);
int term_link_url_is_safe(const char *url);

/* Return one if an OSC 8 opener was emitted, otherwise zero. */
int term_link_fstart(const char *url, FILE *out);
void term_link_fend(FILE *out);

#endif
