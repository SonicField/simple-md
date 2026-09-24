CC ?= cc
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
DESTDIR ?=

CPPFLAGS ?= -Isrc
BASE_CFLAGS = -Wall -Wextra -Wshadow -Werror -std=c11 \
	-D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
CFLAGS ?= -O2
ALL_CFLAGS = $(BASE_CFLAGS) $(CFLAGS)
ANALYZER_CC ?= gcc
ANALYZER_CFLAGS ?= -O0 -g -fanalyzer

TARGET = simple-md
BUILD_DIR = build
ANALYZE_TARGET = $(BUILD_DIR)/simple-md-analyze

HIGHLIGHT_SOURCES = src/md_highlight.c src/md_lang_c.c src/md_lang_js.c \
	src/md_lang_py.c src/md_lang_pas.c
ALLOC_SOURCE = src/sm_alloc.c

SOURCES = src/main.c src/md_ast.c src/md_parse.c src/md_render.c \
	src/md_table.c src/md_style.c src/md_viewport.c src/md_terminal.c \
	src/md_output.c src/md_search.c src/md_outline.c $(HIGHLIGHT_SOURCES) src/term_style.c src/unicode_width.c \
	src/bidi.c src/term_link.c $(ALLOC_SOURCE)

TEST_BINS = $(BUILD_DIR)/test_md_ast $(BUILD_DIR)/test_md_parse \
	$(BUILD_DIR)/test_md_render $(BUILD_DIR)/test_md_table \
	$(BUILD_DIR)/test_md_viewport $(BUILD_DIR)/test_md_highlight \
	$(BUILD_DIR)/test_md_output $(BUILD_DIR)/test_md_search

TEST_BINS += $(BUILD_DIR)/test_md_outline
TEST_BINS += $(BUILD_DIR)/test_term_link

.PHONY: all clean install test unit integration debug sanitize analyze

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CPPFLAGS) $(ALL_CFLAGS) -o $@ $(SOURCES)

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/test_md_ast: tests/test_md_ast.c src/md_ast.c $(ALLOC_SOURCE) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(ALL_CFLAGS) -o $@ $^

$(BUILD_DIR)/test_md_parse: tests/test_md_parse.c src/md_ast.c src/md_parse.c $(ALLOC_SOURCE) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(ALL_CFLAGS) -o $@ $^

$(BUILD_DIR)/test_md_render: tests/test_md_render.c src/md_render.c src/md_parse.c \
	src/md_ast.c src/md_style.c src/md_table.c $(HIGHLIGHT_SOURCES) \
	src/term_style.c src/unicode_width.c $(ALLOC_SOURCE) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(ALL_CFLAGS) -o $@ $^

$(BUILD_DIR)/test_md_table: tests/test_md_table.c src/md_table.c src/md_render.c \
	src/md_parse.c src/md_ast.c src/md_style.c $(HIGHLIGHT_SOURCES) \
	src/term_style.c src/unicode_width.c $(ALLOC_SOURCE) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(ALL_CFLAGS) -o $@ $^

$(BUILD_DIR)/test_md_viewport: tests/test_md_viewport.c src/md_viewport.c \
	src/md_terminal.c src/md_render.c src/md_parse.c src/md_ast.c \
	src/md_style.c src/md_table.c src/md_search.c $(HIGHLIGHT_SOURCES) src/term_style.c \
	src/unicode_width.c src/bidi.c src/term_link.c $(ALLOC_SOURCE) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(ALL_CFLAGS) -o $@ $^

$(BUILD_DIR)/test_md_highlight: tests/test_md_highlight.c $(HIGHLIGHT_SOURCES) \
	src/md_style.c src/term_style.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(ALL_CFLAGS) -o $@ $^

$(BUILD_DIR)/test_md_output: tests/test_md_output.c src/md_output.c \
	src/term_style.c src/term_link.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(ALL_CFLAGS) -o $@ $^

$(BUILD_DIR)/test_md_search: tests/test_md_search.c src/md_search.c $(ALLOC_SOURCE) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(ALL_CFLAGS) -o $@ $^

$(BUILD_DIR)/test_md_outline: tests/test_md_outline.c src/md_outline.c \
	src/md_ast.c src/md_parse.c src/md_render.c src/md_style.c src/md_table.c \
	$(HIGHLIGHT_SOURCES) src/term_style.c src/unicode_width.c $(ALLOC_SOURCE) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(ALL_CFLAGS) -o $@ $^

$(BUILD_DIR)/test_term_link: tests/test_term_link.c src/term_link.c | $(BUILD_DIR)
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

analyze: $(ANALYZE_TARGET)

$(ANALYZE_TARGET): $(SOURCES) | $(BUILD_DIR)
	$(ANALYZER_CC) $(CPPFLAGS) $(BASE_CFLAGS) $(ANALYZER_CFLAGS) -o $@ $(SOURCES)

clean:
	rm -f $(TARGET) $(TEST_BINS) $(ANALYZE_TARGET)
	rmdir $(BUILD_DIR) 2>/dev/null || true
