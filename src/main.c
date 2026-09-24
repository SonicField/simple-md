/*
 * main.c — Entry point for simple-md.
 *
 * Pipeline: stdin -> parse -> render -> viewport loop.
 * Reads markdown from stdin, parses it into an AST, renders to
 * styled display lines, and presents a scrollable pager in the terminal.
 */

#define _POSIX_C_SOURCE 200809L

#include "md_parse.h"
#include "md_render.h"
#include "md_output.h"
#include "md_viewport.h"
#include "md_terminal.h"
#include "sm_assert.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef enum {
    PAGER_AUTO,
    PAGER_ALWAYS,
    PAGER_NEVER
} pager_mode_t;

static void print_usage(FILE *out) {
    fputs("Usage: simple-md [--width=COLUMNS] [--pager=MODE] [FILE]\n"
          "       simple-md --help\n"
          "\n"
          "Read Markdown from FILE, or from standard input when FILE is '-' or omitted.\n"
          "Pager MODE is auto, always, or never (default: auto).\n"
          "Press q or Escape to quit.\n", out);
}

static int parse_width(const char *value, int *width) {
    char *end = NULL;
    errno = 0;
    long parsed = strtol(value, &end, 10);
    if (errno != 0 || end == value || *end != '\0' || parsed < 1 ||
        parsed > 10000 || parsed > INT_MAX) {
        return -1;
    }
    *width = (int)parsed;
    return 0;
}

/* Read a stream into a malloc'd NUL-terminated buffer.
 * Returns NULL on allocation failure. */
static char *read_stream(FILE *stream) {
    size_t cap = 65536;  /* 64 KB initial allocation */
    size_t len = 0;
    char *buf = malloc(cap);
    if (!buf) return NULL;

    for (;;) {
        size_t avail = cap - len;
        if (avail == 0) {
            size_t newcap = cap * 2;
            char *newbuf = realloc(buf, newcap);
            if (!newbuf) {
                free(buf);
                return NULL;
            }
            buf = newbuf;
            cap = newcap;
            avail = cap - len;
        }
        size_t n = fread(buf + len, 1, avail, stream);
        len += n;
        if (n < avail) {
            /* EOF or error */
            if (ferror(stream)) {
                free(buf);
                return NULL;
            }
            break;
        }
    }

    /* NUL-terminate, ensuring space */
    if (len == cap) {
        char *newbuf = realloc(buf, cap + 1);
        if (!newbuf) {
            free(buf);
            return NULL;
        }
        buf = newbuf;
    }
    buf[len] = '\0';
    return buf;
}

int main(int argc, char *argv[]) {
    int force_cols = 0;
    pager_mode_t pager_mode = PAGER_AUTO;
    const char *input_path = NULL;
    int options_done = 0;

    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];
        if (!options_done && strcmp(arg, "--") == 0) {
            options_done = 1;
        } else if (!options_done && (strcmp(arg, "-h") == 0 ||
                                     strcmp(arg, "--help") == 0)) {
            print_usage(stdout);
            return 0;
        } else if (!options_done && (strcmp(arg, "-V") == 0 ||
                                     strcmp(arg, "--version") == 0)) {
            puts("simple-md 0.1.0");
            return 0;
        } else if (!options_done && strncmp(arg, "--width=", 8) == 0) {
            if (parse_width(arg + 8, &force_cols) != 0) {
                fprintf(stderr, "simple-md: invalid width: %s\n", arg + 8);
                return 2;
            }
        } else if (!options_done && strncmp(arg, "--pager=", 8) == 0) {
            const char *mode = arg + 8;
            if (strcmp(mode, "auto") == 0) pager_mode = PAGER_AUTO;
            else if (strcmp(mode, "always") == 0) pager_mode = PAGER_ALWAYS;
            else if (strcmp(mode, "never") == 0) pager_mode = PAGER_NEVER;
            else {
                fprintf(stderr, "simple-md: invalid pager mode: %s\n", mode);
                return 2;
            }
        } else if (!options_done && arg[0] == '-' && strcmp(arg, "-") != 0) {
            fprintf(stderr, "simple-md: unknown option: %s\n", arg);
            print_usage(stderr);
            return 2;
        } else if (input_path != NULL) {
            fprintf(stderr, "simple-md: only one input file may be specified\n");
            return 2;
        } else {
            input_path = arg;
        }
    }

    FILE *input_stream = stdin;
    if (input_path != NULL && strcmp(input_path, "-") != 0) {
        input_stream = fopen(input_path, "rb");
        if (input_stream == NULL) {
            fprintf(stderr, "simple-md: cannot open '%s': %s\n",
                    input_path, strerror(errno));
            return 1;
        }
    }

    char *input = read_stream(input_stream);
    if (input_stream != stdin) fclose(input_stream);
    if (!input) {
        fprintf(stderr, "simple-md: failed to read input\n");
        return 1;
    }
    if (input[0] == '\0') {
        free(input);
        return 0;
    }

    /* 2. Parse into AST */
    md_block_node_t *doc = md_parse(input);
    free(input);  /* parser made its own copies */

    /* Redirected output must remain useful in pipelines and must never wait
     * for terminal input. */
    int output_is_tty = isatty(STDOUT_FILENO);
    if (!output_is_tty) {
        if (pager_mode == PAGER_ALWAYS) {
            fprintf(stderr, "simple-md: cannot page when output is not a terminal\n");
            md_block_destroy(doc);
            return 1;
        }
        int cols = force_cols > 0 ? force_cols : 80;
        md_layout_t *plain_layout = md_render(doc, cols);
        int write_status = md_output_write_plain(stdout, plain_layout);
        md_layout_destroy(plain_layout);
        md_block_destroy(doc);
        if (write_status != 0) {
            fprintf(stderr, "simple-md: failed to write output\n");
            return 1;
        }
        return 0;
    }

    int rows = 24;
    int cols = 80;
    md_terminal_get_size(&rows, &cols);
    if (force_cols > 0) cols = force_cols;

    md_layout_t *layout = md_render(doc, cols);
    if (pager_mode == PAGER_NEVER ||
        (pager_mode == PAGER_AUTO && layout->line_count <= rows - 1)) {
        int write_status = md_output_write_styled(stdout, layout);
        md_layout_destroy(layout);
        md_block_destroy(doc);
        if (write_status != 0) {
            fprintf(stderr, "simple-md: failed to write output\n");
            return 1;
        }
        return 0;
    }

    /* 3. Enter raw mode first (opens /dev/tty for correct terminal queries) */
    if (md_terminal_enter_raw() != 0) {
        fprintf(stderr, "simple-md: failed to enter raw mode\n");
        md_layout_destroy(layout);
        md_block_destroy(doc);
        return 1;
    }

    /* 4. Get terminal size (now uses /dev/tty via tty_fd) */
    if (md_terminal_get_size(&rows, &cols) != 0) {
        rows = 24;
        cols = 80;
    }

    if (force_cols > 0) cols = force_cols;

    /* 5. Re-render if entering raw mode changed the detected width. */
    md_layout_destroy(layout);
    layout = md_render(doc, cols);

    /* 6. Initialize viewport and draw initial screen */
    md_view_state_t vs;
    memset(&vs, 0, sizeof(vs));
    md_viewport_init(&vs, rows, cols, layout->line_count);
    md_viewport_draw(&vs, layout);
    fflush(stdout);

    /* 7. Event loop */
    while (1) {
        /* Check for terminal resize */
        if (md_terminal_resize_pending()) {
            md_terminal_get_size(&rows, &cols);
            md_layout_destroy(layout);
            layout = md_render(doc, cols);
            int saved_offset = vs.scroll_offset;
            md_viewport_init(&vs, rows, cols, layout->line_count);
            vs.scroll_offset = saved_offset;
            /* Clamp scroll offset to new bounds */
            int max_off = vs.total_lines - vs.visible_rows;
            if (max_off < 0) max_off = 0;
            if (vs.scroll_offset > max_off) vs.scroll_offset = max_off;
            /* Clear entire screen and reset cursor for clean redraw */
            fputs("\033[2J\033[H", stdout);
            fflush(stdout);
            md_viewport_draw(&vs, layout);
            fflush(stdout);
        }

        md_key_t key = md_terminal_read_key();
        int need_draw = 1;

        switch (key) {
        case MD_KEY_QUIT:
            goto done;
        case MD_KEY_UP:
            md_viewport_scroll_up(&vs, 1);
            break;
        case MD_KEY_DOWN:
            md_viewport_scroll_down(&vs, 1);
            break;
        case MD_KEY_PAGE_UP:
            md_viewport_page_up(&vs);
            break;
        case MD_KEY_PAGE_DOWN:
            md_viewport_page_down(&vs);
            break;
        case MD_KEY_HOME:
            md_viewport_home(&vs);
            break;
        case MD_KEY_END:
            md_viewport_end(&vs);
            break;
        case MD_KEY_RIGHT:
            md_viewport_pan_right(&vs);
            break;
        case MD_KEY_LEFT:
            md_viewport_pan_left(&vs);
            break;
        case MD_KEY_HELP:
            md_viewport_draw_help(&vs);
            /* Wait for Enter to return */
            while (1) {
                md_key_t hk = md_terminal_read_key();
                if (hk == MD_KEY_ENTER || hk == MD_KEY_HELP || hk == MD_KEY_QUIT)
                    break;
            }
            break;
        case MD_KEY_ENTER:
            need_draw = 0;
            break;
        default:
            need_draw = 0;
            break;
        }

        if (need_draw) {
            md_viewport_draw(&vs, layout);
            fflush(stdout);
        }
    }

done:
    md_terminal_leave_raw();
    md_layout_destroy(layout);
    md_block_destroy(doc);
    return 0;
}
