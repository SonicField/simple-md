/*
 * md_style.h — Style palette for markdown rendering.
 *
 * Defines term_style_t constants for all markdown element types.
 * Extends the term_style colour model (256-colour, attributes).
 */

#ifndef MD_STYLE_H
#define MD_STYLE_H

#include "term_style.h"

extern const term_style_t MD_STYLE_H1;
extern const term_style_t MD_STYLE_H2;
extern const term_style_t MD_STYLE_H3;
extern const term_style_t MD_STYLE_H4;
extern const term_style_t MD_STYLE_BODY;
extern const term_style_t MD_STYLE_BOLD;
extern const term_style_t MD_STYLE_ITALIC;
extern const term_style_t MD_STYLE_BOLD_ITALIC;
extern const term_style_t MD_STYLE_INLINE_CODE;
extern const term_style_t MD_STYLE_CODE_FENCE;
extern const term_style_t MD_STYLE_CODE_BORDER;
extern const term_style_t MD_STYLE_LINK_TEXT;
extern const term_style_t MD_STYLE_HRULE;
extern const term_style_t MD_STYLE_TABLE_BORDER;
extern const term_style_t MD_STYLE_TABLE_HEADER;
extern const term_style_t MD_STYLE_TABLE_CELL;
extern const term_style_t MD_STYLE_LIST_MARKER;
extern const term_style_t MD_STYLE_BLOCKQUOTE_BAR;
extern const term_style_t MD_STYLE_BLOCKQUOTE_TEXT;
extern const term_style_t MD_STYLE_STATUS_BAR;

/* Syntax highlighting */
extern const term_style_t MD_STYLE_HL_KEYWORD;
extern const term_style_t MD_STYLE_HL_TYPE;
extern const term_style_t MD_STYLE_HL_STRING;
extern const term_style_t MD_STYLE_HL_NUMBER;
extern const term_style_t MD_STYLE_HL_COMMENT;
extern const term_style_t MD_STYLE_HL_PREPROC;
extern const term_style_t MD_STYLE_HL_OPERATOR;

#endif /* MD_STYLE_H */
