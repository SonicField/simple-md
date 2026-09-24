#include "md_output.h"
#include "term_style.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void fail(const char *message) {
    fprintf(stderr, "FAIL: %s\n", message);
    exit(1);
}

int main(void) {
    md_span_t first_spans[] = {
        { .text = "Hello", .style = { 1, 2, TERM_ATTR_BOLD }, .width = 5 },
        { .text = " world", .style = { 3, 4, TERM_ATTR_UNDERLINE }, .width = 6 },
    };
    md_display_line_t lines[] = {
        { .spans = first_spans, .span_count = 2, .display_width = 11 },
        { .spans = NULL, .span_count = 0, .display_width = 0 },
    };
    md_layout_t layout = { .lines = lines, .line_count = 2, .max_width = 11 };
    FILE *output = tmpfile();
    if (output == NULL) fail("tmpfile failed");

    if (md_output_write_plain(output, &layout) != 0) fail("write failed");
    rewind(output);

    char actual[256] = {0};
    size_t length = fread(actual, 1, sizeof(actual) - 1, output);
    fclose(output);

    if (length != 13 || strcmp(actual, "Hello world\n\n") != 0)
        fail("plain output did not preserve lines and remove styling");
    if (strchr(actual, '\033') != NULL) fail("plain output contained escape bytes");

    output = tmpfile();
    if (output == NULL) fail("second tmpfile failed");
    if (md_output_write_styled(output, &layout) != 0) fail("styled write failed");
    rewind(output);
    memset(actual, 0, sizeof(actual));
    length = fread(actual, 1, sizeof(actual) - 1, output);
    fclose(output);
    if (length == 0 || strstr(actual, "\033[") == NULL)
        fail("styled output omitted SGR sequences");
    if (strstr(actual, "Hello") == NULL || strstr(actual, "world") == NULL)
        fail("styled output omitted text");

    output = tmpfile();
    if (output == NULL) fail("third tmpfile failed");
    term_style_set_enabled(0);
    if (md_output_write_styled(output, &layout) != 0)
        fail("color-disabled write failed");
    term_style_set_enabled(1);
    rewind(output);
    memset(actual, 0, sizeof(actual));
    length = fread(actual, 1, sizeof(actual) - 1, output);
    fclose(output);
    if (length != 13 || strcmp(actual, "Hello world\n\n") != 0)
        fail("color-disabled output was not plain styled text");

    puts("test_md_output: PASS");
    return 0;
}
