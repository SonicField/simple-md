#ifndef MD_OUTPUT_H
#define MD_OUTPUT_H

#include "md_render.h"

#include <stdio.h>

/* Write rendered text without terminal control sequences. */
int md_output_write_plain(FILE *out, const md_layout_t *layout);

/* Write rendered text with SGR styling, without pager control sequences. */
int md_output_write_styled(FILE *out, const md_layout_t *layout);

#endif
