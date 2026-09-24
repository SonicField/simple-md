#include "sm_alloc.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void allocation_failed(const char *operation, size_t size) {
    fprintf(stderr, "simple-md: %s failed for %zu bytes\n", operation, size);
    abort();
}

void *sm_malloc(size_t size) {
    size_t requested = size == 0 ? 1 : size;
    void *result = malloc(requested);
    if (result == NULL) allocation_failed("malloc", requested);
    return result;
}

void *sm_calloc(size_t count, size_t size) {
    size_t actual_count = count == 0 ? 1 : count;
    size_t actual_size = size == 0 ? 1 : size;
    void *result = calloc(actual_count, actual_size);
    if (result == NULL) {
        size_t requested = actual_count > SIZE_MAX / actual_size
            ? SIZE_MAX : actual_count * actual_size;
        allocation_failed("calloc", requested);
    }
    return result;
}

void *sm_realloc(void *pointer, size_t size) {
    size_t requested = size == 0 ? 1 : size;
    void *result = realloc(pointer, requested);
    if (result == NULL) allocation_failed("realloc", requested);
    return result;
}

char *sm_strdup(const char *text) {
    if (text == NULL) {
        fputs("simple-md: strdup received NULL\n", stderr);
        abort();
    }
    size_t length = strlen(text) + 1;
    char *copy = sm_malloc(length);
    memcpy(copy, text, length);
    return copy;
}
