/* CommonMark 0.31.2 parser adapter. MD4C provides parsing; this file owns AST conversion. */
#include "md_parse.h"
#include "sm_alloc.h"
#include "sm_assert.h"
#include "md4c.h"
#include "entity.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

typedef struct { md_block_node_t *node; } block_frame_t;
typedef struct { md_inline_node_t *node; } inline_frame_t;
typedef struct {
    md_block_node_t *root;
    block_frame_t *blocks; size_t block_count, block_cap;
    inline_frame_t *inlines; size_t inline_count, inline_cap;
} parse_state_t;

static char *copy_text(const char *text, size_t size) {
    char *copy = sm_malloc(size + 1);
    memcpy(copy, text, size); copy[size] = '\0';
    return copy;
}
static void append_text(char **target, const char *text, size_t size) {
    size_t old = *target ? strlen(*target) : 0;
    *target = sm_realloc(*target, old + size + 1);
    memcpy(*target + old, text, size); (*target)[old + size] = '\0';
}
static void push_block(parse_state_t *s, md_block_node_t *node) {
    if (s->block_count == s->block_cap) {
        s->block_cap = s->block_cap ? s->block_cap * 2 : 16;
        s->blocks = sm_realloc(s->blocks, s->block_cap * sizeof(*s->blocks));
    }
    s->blocks[s->block_count++].node = node;
}
static md_block_node_t *current_block(parse_state_t *s) {
    for (size_t i = s->block_count; i > 0; i--)
        if (s->blocks[i-1].node) return s->blocks[i-1].node;
    return s->root;
}
static void push_inline(parse_state_t *s, md_inline_node_t *node) {
    if (s->inline_count == s->inline_cap) {
        s->inline_cap = s->inline_cap ? s->inline_cap * 2 : 16;
        s->inlines = sm_realloc(s->inlines, s->inline_cap * sizeof(*s->inlines));
    }
    s->inlines[s->inline_count++].node = node;
}
static md_inline_node_t *current_inline(parse_state_t *s) {
    return s->inline_count ? s->inlines[s->inline_count-1].node : NULL;
}
static void add_inline(parse_state_t *s, md_inline_node_t *node) {
    md_inline_node_t *parent = current_inline(s);
    if (parent) md_inline_add_child(parent, node);
    else {
        md_block_node_t *block = current_block(s);
        /* MD4C deliberately omits paragraph callbacks in tight list items.
         * simple-md's renderer uses an explicit paragraph node, so restore
         * that representational detail without changing parse semantics. */
        if (block->type == MD_BLOCK_LIST_ITEM) {
            md_block_node_t *paragraph = block->children;
            while (paragraph && paragraph->next) paragraph = paragraph->next;
            if (!paragraph || paragraph->type != MD_BLOCK_PARAGRAPH) {
                paragraph = md_block_create(MD_BLOCK_PARAGRAPH);
                md_block_add_child(block, paragraph);
            }
            block = paragraph;
        }
        md_block_add_inline(block, node);
    }
}
static int list_depth(parse_state_t *s) {
    int depth = 0;
    for (size_t i = 0; i < s->block_count; i++)
        if (s->blocks[i].node && s->blocks[i].node->type == MD_BLOCK_LIST) depth++;
    return depth;
}
static md_block_node_t *find_table(parse_state_t *s) {
    for (size_t i = s->block_count; i > 0; i--)
        if (s->blocks[i-1].node && s->blocks[i-1].node->type == MD_BLOCK_TABLE)
            return s->blocks[i-1].node;
    return NULL;
}

static int enter_block(MD_BLOCKTYPE type, void *detail, void *userdata) {
    parse_state_t *s = userdata;
    md_block_node_t *node = NULL;
    switch (type) {
    case MD4C_BLOCK_DOC: push_block(s, s->root); return 0;
    case MD4C_BLOCK_QUOTE: node = md_block_create(MD_BLOCK_BLOCKQUOTE); break;
    case MD4C_BLOCK_UL: {
        MD4C_BLOCK_UL_DETAIL *d = detail;
        node = md_block_create(MD_BLOCK_LIST); node->ordered = 0;
        node->level = list_depth(s) + 1; node->is_tight = d->is_tight; break;
    }
    case MD4C_BLOCK_OL: {
        MD4C_BLOCK_OL_DETAIL *d = detail;
        node = md_block_create(MD_BLOCK_LIST); node->ordered = 1;
        node->start = (int)d->start; node->level = list_depth(s) + 1;
        node->is_tight = d->is_tight; break;
    }
    case MD4C_BLOCK_LI:
        node = md_block_create(MD_BLOCK_LIST_ITEM); node->level = list_depth(s); break;
    case MD4C_BLOCK_HR: node = md_block_create(MD_BLOCK_HRULE); break;
    case MD4C_BLOCK_H: {
        MD4C_BLOCK_H_DETAIL *d = detail;
        node = md_block_create(MD_BLOCK_HEADING); node->level = (int)d->level; break;
    }
    case MD4C_BLOCK_CODE: {
        MD4C_BLOCK_CODE_DETAIL *d = detail;
        node = md_block_create(MD_BLOCK_CODE_FENCE);
        node->language = copy_text(d->lang.text, d->lang.size); break;
    }
    case MD4C_BLOCK_HTML: node = md_block_create(MD_BLOCK_HTML); break;
    case MD4C_BLOCK_P: node = md_block_create(MD_BLOCK_PARAGRAPH); break;
    case MD4C_BLOCK_TABLE: {
        MD4C_BLOCK_TABLE_DETAIL *d = detail;
        node = md_block_create(MD_BLOCK_TABLE); node->col_count = (int)d->col_count;
        node->col_align = sm_calloc(d->col_count, sizeof(*node->col_align)); break;
    }
    case MD4C_BLOCK_THEAD:
    case MD4C_BLOCK_TBODY: push_block(s, NULL); return 0;
    case MD4C_BLOCK_TR: node = md_block_create(MD_BLOCK_TABLE_ROW); break;
    case MD4C_BLOCK_TH:
    case MD4C_BLOCK_TD: {
        MD4C_BLOCK_TD_DETAIL *d = detail;
        node = md_block_create(MD_BLOCK_TABLE_CELL); node->is_header = type == MD4C_BLOCK_TH;
        node->align = d->align == MD4C_ALIGN_CENTER ? MD_ALIGN_CENTRE :
                      d->align == MD4C_ALIGN_RIGHT ? MD_ALIGN_RIGHT : MD_ALIGN_LEFT;
        md_block_node_t *row = current_block(s);
        if (row->type == MD_BLOCK_TABLE_ROW && type == MD4C_BLOCK_TH) row->is_header = 1;
        md_block_node_t *table = find_table(s);
        if (table && table->col_align) {
            int col = 0;
            for (md_block_node_t *c = row->children; c; c = c->next) col++;
            if (col < table->col_count) table->col_align[col] = node->align;
        }
        break;
    }
    }
    ASSERT_MSG(node != NULL, "CommonMark block callback has no AST mapping: %d", type);
    md_block_add_child(current_block(s), node); push_block(s, node); return 0;
}
static int leave_block(MD_BLOCKTYPE type, void *detail, void *userdata) {
    (void)detail; parse_state_t *s = userdata;
    ASSERT_MSG(s->block_count > 0, "CommonMark block stack underflow: %d", type);
    s->block_count--; return 0;
}
static char *copy_attribute(const MD_ATTRIBUTE *a) { return copy_text(a->text, a->size); }
static int enter_span(MD_SPANTYPE type, void *detail, void *userdata) {
    parse_state_t *s = userdata; md_inline_node_t *node;
    switch (type) {
    case MD4C_SPAN_EM: node = md_inline_create(MD_INLINE_ITALIC); break;
    case MD4C_SPAN_STRONG: node = md_inline_create(MD_INLINE_BOLD); break;
    case MD4C_SPAN_A: {
        MD4C_SPAN_A_DETAIL *d = detail; node = md_inline_create(MD_INLINE_LINK);
        node->url = copy_attribute(&d->href); node->title = copy_attribute(&d->title); break;
    }
    case MD4C_SPAN_IMG: {
        MD4C_SPAN_IMG_DETAIL *d = detail; node = md_inline_create(MD_INLINE_IMAGE);
        node->url = copy_attribute(&d->src); node->title = copy_attribute(&d->title); break;
    }
    case MD4C_SPAN_CODE: node = md_inline_create(MD_INLINE_CODE); break;
    default: node = md_inline_create(MD_INLINE_TEXT); break;
    }
    add_inline(s, node); push_inline(s, node); return 0;
}
static int leave_span(MD_SPANTYPE type, void *detail, void *userdata) {
    (void)detail; parse_state_t *s = userdata;
    ASSERT_MSG(s->inline_count > 0, "CommonMark inline stack underflow: %d", type);
    s->inline_count--; return 0;
}
static size_t encode_utf8(unsigned cp, char out[4]) {
    if (cp <= 0x7f) { out[0]=(char)cp; return 1; }
    if (cp <= 0x7ff) { out[0]=(char)(0xc0|(cp>>6)); out[1]=(char)(0x80|(cp&63)); return 2; }
    if (cp <= 0xffff) { out[0]=(char)(0xe0|(cp>>12)); out[1]=(char)(0x80|((cp>>6)&63)); out[2]=(char)(0x80|(cp&63)); return 3; }
    out[0]=(char)(0xf0|(cp>>18)); out[1]=(char)(0x80|((cp>>12)&63));
    out[2]=(char)(0x80|((cp>>6)&63)); out[3]=(char)(0x80|(cp&63)); return 4;
}
static void add_codepoint(parse_state_t *s, unsigned cp) {
    char bytes[4]; size_t size = encode_utf8(cp, bytes);
    md_inline_node_t *node = md_inline_create(MD_INLINE_TEXT);
    node->text = copy_text(bytes, size); add_inline(s, node);
}
static void add_entity(parse_state_t *s, const char *text, size_t size) {
    if (size >= 4 && text[1] == '#') {
        unsigned cp = 0, base = 10; size_t pos = 2;
        if (pos < size && (text[pos]=='x' || text[pos]=='X')) { base=16; pos++; }
        for (; pos + 1 < size; pos++) {
            unsigned d;
            if (text[pos]>='0' && text[pos]<='9') d=(unsigned)(text[pos]-'0');
            else if (base==16 && text[pos]>='a' && text[pos]<='f') d=(unsigned)(text[pos]-'a'+10);
            else if (base==16 && text[pos]>='A' && text[pos]<='F') d=(unsigned)(text[pos]-'A'+10);
            else break;
            cp=cp*base+d;
        }
        if (!cp || cp>0x10ffff || (cp>=0xd800 && cp<=0xdfff)) cp=0xfffd;
        add_codepoint(s, cp); return;
    }
    const ENTITY *ent = entity_lookup(text, size);
    if (!ent) {
        md_inline_node_t *node=md_inline_create(MD_INLINE_TEXT);
        node->text=copy_text(text,size); add_inline(s,node); return;
    }
    add_codepoint(s,ent->codepoints[0]); if (ent->codepoints[1]) add_codepoint(s,ent->codepoints[1]);
}
static int add_text(MD_TEXTTYPE type, const MD_CHAR *text, MD_SIZE size, void *userdata) {
    parse_state_t *s=userdata; md_block_node_t *block=current_block(s);
    if (block->type==MD_BLOCK_CODE_FENCE || block->type==MD_BLOCK_HTML) {
        append_text(&block->raw,text,size); return 0;
    }
    if (type==MD_TEXT_BR || type==MD_TEXT_SOFTBR) {
        add_inline(s,md_inline_create(type==MD_TEXT_BR?MD_INLINE_HARDBREAK:MD_INLINE_SOFTBREAK)); return 0;
    }
    if (type==MD_TEXT_NULLCHAR) { add_codepoint(s,0xfffd); return 0; }
    if (type==MD_TEXT_ENTITY) { add_entity(s,text,size); return 0; }
    md_inline_node_t *parent = current_inline(s);
    if (type==MD_TEXT_CODE && parent && parent->type==MD_INLINE_CODE) {
        append_text(&parent->text,text,size); return 0;
    }
    md_inline_node_t *node=md_inline_create(type==MD_TEXT_CODE?MD_INLINE_CODE_TEXT:
                                            type==MD_TEXT_HTML?MD_INLINE_HTML:MD_INLINE_TEXT);
    node->text=copy_text(text,size); add_inline(s,node); return 0;
}
static void collapse_emphasis(md_inline_node_t *node) {
    for (; node; node=node->next) {
        collapse_emphasis(node->children);
        if ((node->type==MD_INLINE_BOLD || node->type==MD_INLINE_ITALIC) &&
            node->children && !node->children->next &&
            ((node->type==MD_INLINE_BOLD && node->children->type==MD_INLINE_ITALIC) ||
             (node->type==MD_INLINE_ITALIC && node->children->type==MD_INLINE_BOLD))) {
            md_inline_node_t *child=node->children; node->type=MD_INLINE_BOLD_ITALIC;
            node->children=child->children; child->children=NULL; child->next=NULL; md_inline_destroy(child);
        }
    }
}
static void finish_tree(md_block_node_t *block) {
    for (; block; block=block->next) { collapse_emphasis(block->inlines); finish_tree(block->children); }
}
md_block_node_t *md_parse(const char *input) {
    const char *source=input?input:""; size_t size=strlen(source);
    md_block_node_t *root=md_block_create(MD_BLOCK_DOCUMENT);
    if (size>UINT_MAX) return root;
    parse_state_t state; memset(&state,0,sizeof(state)); state.root=root;
    const MD_PARSER parser={0,MD_FLAG_TABLES,enter_block,leave_block,enter_span,leave_span,add_text,NULL,NULL};
    int result=md4c_parse(source,(MD_SIZE)size,&parser,&state);
    free(state.blocks); free(state.inlines);
    ASSERT_MSG(result==0,"CommonMark parser failed: result=%d, input_size=%zu",result,size);
    ASSERT_MSG(state.block_count==0,"CommonMark parser left %zu block frames",state.block_count);
    ASSERT_MSG(state.inline_count==0,"CommonMark parser left %zu inline frames",state.inline_count);
    finish_tree(root); return root;
}
