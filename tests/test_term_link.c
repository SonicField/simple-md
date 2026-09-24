#include "term_link.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void fail(const char *message) {
    fprintf(stderr, "FAIL: %s\n", message);
    exit(1);
}

static size_t capture_start(const char *url, char *buffer, size_t capacity) {
    FILE *output = tmpfile();
    if (output == NULL) fail("tmpfile failed");
    term_link_fstart(url, output);
    fflush(output);
    rewind(output);
    size_t length = fread(buffer, 1, capacity - 1, output);
    buffer[length] = '\0';
    fclose(output);
    return length;
}

int main(void) {
    char output[256];
    size_t length = capture_start("https://example.com/a?b=c", output, sizeof(output));
    if (length == 0 || strstr(output, "\033]8;;https://example.com/a?b=c\033\\") == NULL)
        fail("safe URL did not produce an OSC 8 opener");

    if (capture_start("https://bad.example/\033]8;;evil", output, sizeof(output)) != 0)
        fail("Escape injection was not rejected");
    if (capture_start("https://bad.example/\aevil", output, sizeof(output)) != 0)
        fail("BEL injection was not rejected");
    if (capture_start("", output, sizeof(output)) != 0)
        fail("empty URL was not rejected");

    term_link_set_enabled(0);
    if (capture_start("https://example.com", output, sizeof(output)) != 0)
        fail("disabled hyperlinks still emitted output");

    puts("test_term_link: PASS");
    return 0;
}
