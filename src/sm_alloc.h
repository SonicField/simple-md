#ifndef SIMPLE_MD_ALLOC_H
#define SIMPLE_MD_ALLOC_H

#include <stddef.h>

/*
 * Allocations used for required document state. These functions either return
 * usable storage or terminate with a diagnostic; callers never receive NULL.
 */
void *sm_malloc(size_t size);
void *sm_calloc(size_t count, size_t size);
void *sm_realloc(void *pointer, size_t size);
char *sm_strdup(const char *text);

#endif /* SIMPLE_MD_ALLOC_H */
