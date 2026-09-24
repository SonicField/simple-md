CC ?= cc
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
DESTDIR ?=

CPPFLAGS ?= -Isrc
BASE_CFLAGS = -Wall -Wextra -Wshadow -Werror -std=c11 \
	-D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
CFLAGS ?= -O2
ALL_CFLAGS = $(BASE_CFLAGS) $(CFLAGS)

TARGET = simple-md
BUILD_DIR = build

HIGHLIGHT_SOURCES = src/md_highlight.c src/md_lang_c.c src/md_lang_js.c \
	src/md_lang_py.c src/md_lang_pas.c

SOURCES = src/main.c src/md_ast.c src/md_parse.c src/md_render.c \
	src/md_table.c src/md_style.c src/md_viewport.c src/md_terminal.c \
	src/md_output.c $(HIGHLIGHT_SOURCES) src/term_style.c src/unicode_width.c \
	src/bidi.c

TEST_BINS = $(BUILD_DIR)/test_md_ast $(BUILD_DIR)/test_md_parse \
	$(BUILD_DIR)/test_md_render $(BUILD_DIR)/test_md_table \
	$(BUILD_DIR)/test_md_viewport $(BUILD_DIR)/test_md_highlight \
	$(BUILD_DIR)/test_md_output

.PHONY: all clean install test unit integration debug sanitize

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CPPFLAGS) $(ALL_CFLAGS) -o $@ $(SOURCES)

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/test_md_ast: tests/test_md_ast.c src/md_ast.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(ALL_CFLAGS) -o $@ $^

$(BUILD_DIR)/test_md_parse: tests/test_md_parse.c src/md_ast.c src/md_parse.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(ALL_CFLAGS) -o $@ $^

$(BUILD_DIR)/test_md_render: tests/test_md_render.c src/md_render.c src/md_parse.c \
	src/md_ast.c src/md_style.c src/md_table.c $(HIGHLIGHT_SOURCES) \
	src/term_style.c src/unicode_width.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(ALL_CFLAGS) -o $@ $^

$(BUILD_DIR)/test_md_table: tests/test_md_table.c src/md_table.c src/md_render.c \
	src/md_parse.c src/md_ast.c src/md_style.c $(HIGHLIGHT_SOURCES) \
	src/term_style.c src/unicode_width.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(ALL_CFLAGS) -o $@ $^

$(BUILD_DIR)/test_md_viewport: tests/test_md_viewport.c src/md_viewport.c \
	src/md_terminal.c src/md_render.c src/md_parse.c src/md_ast.c \
	src/md_style.c src/md_table.c $(HIGHLIGHT_SOURCES) src/term_style.c \
	src/unicode_width.c src/bidi.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(ALL_CFLAGS) -o $@ $^

$(BUILD_DIR)/test_md_highlight: tests/test_md_highlight.c $(HIGHLIGHT_SOURCES) \
	src/md_style.c src/term_style.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(ALL_CFLAGS) -o $@ $^

$(BUILD_DIR)/test_md_output: tests/test_md_output.c src/md_output.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(ALL_CFLAGS) -o $@ $^

unit: $(TEST_BINS)
	@set -e; for test_bin in $(TEST_BINS); do ./$$test_bin; done

integration: $(TARGET)
	./tests/test_cli.sh
	python3 ./tests/test_terminal.py

test: unit integration

install: $(TARGET)
	install -d "$(DESTDIR)$(BINDIR)"
	install -m 0755 $(TARGET) "$(DESTDIR)$(BINDIR)/$(TARGET)"

debug:
	$(MAKE) clean
	$(MAKE) CFLAGS='-O0 -g3' all

sanitize:
	$(MAKE) clean
	$(MAKE) CFLAGS='-O1 -g3 -fsanitize=address,undefined -fno-omit-frame-pointer' unit

clean:
	rm -f $(TARGET) $(TEST_BINS)
	rmdir $(BUILD_DIR) 2>/dev/null || true
