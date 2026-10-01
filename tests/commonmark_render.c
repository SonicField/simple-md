#include "md4c-html.h"
#include <stdio.h>
#include <stdlib.h>

static void output(const MD_CHAR *text, MD_SIZE size, void *userdata) {
    (void)userdata;
    if (fwrite(text, 1, size, stdout) != size) exit(2);
}

int main(void) {
    char *input = NULL;
    size_t size = 0, capacity = 0, count;
    char buffer[4096];
    while ((count = fread(buffer, 1, sizeof(buffer), stdin)) > 0) {
        if (size + count > capacity) {
            capacity = capacity ? capacity * 2 : 8192;
            while (capacity < size + count) capacity *= 2;
            char *grown = realloc(input, capacity);
            if (!grown) { free(input); return 2; }
            input = grown;
        }
        for (size_t i = 0; i < count; i++) input[size + i] = buffer[i];
        size += count;
    }
    if (ferror(stdin)) { free(input); return 2; }
    int result = md_html(input ? input : "", (MD_SIZE)size, output, NULL,
                         MD_DIALECT_COMMONMARK, 0);
    free(input);
    return result == 0 ? 0 : 2;
}
