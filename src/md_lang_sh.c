/*
 * md_lang_sh.c — Lightweight Bourne-shell-family syntax highlighting.
 *
 * POSIX sh, Bash, Zsh, and Ksh share enough visible syntax for one useful
 * Markdown highlighter: control words, comments, quotes, parameter expansion,
 * pipelines, and redirections. This intentionally does not parse commands,
 * validate a particular dialect, or implement here-document bodies.
 */

#include "md_highlight.h"

#include <ctype.h>
#include <string.h>

static const char *shell_keywords[] = {
    "always", "case", "coproc", "do", "done",
    "elif", "else", "esac", "fi", "for",
    "foreach", "function", "if", "in", "repeat",
    "select", "then", "time", "until", "while",
    NULL
};

static int shell_keyword(const char *word, int length) {
    for (int i = 0; shell_keywords[i] != NULL; i++) {
        if ((int)strlen(shell_keywords[i]) == length &&
            memcmp(word, shell_keywords[i], (size_t)length) == 0) return 1;
    }
    return 0;
}

static int shell_emit(md_hl_span_t *spans, int *count, int capacity,
                      int start, int length, md_hl_token_t token) {
    if (*count >= capacity) return 0;
    spans[*count] = (md_hl_span_t) {
        .start = start,
        .len = length,
        .token = token,
    };
    (*count)++;
    return 1;
}

static int shell_comment_start(const char *line, int pos) {
    if (pos == 0) return 1;
    unsigned char previous = (unsigned char)line[pos - 1];
    return isspace(previous) || strchr(";|&()<>", previous) != NULL;
}

static int shell_quote_end(const char *line, int start, int length, char quote) {
    for (int i = start; i < length; i++) {
        if (quote != '\'' && line[i] == '\\' && i + 1 < length) {
            i++;
            continue;
        }
        if (line[i] == quote) return i + 1;
    }
    return -1;
}

static int shell_variable_len(const char *line, int pos, int length) {
    if (line[pos] != '$' || pos + 1 >= length) return 0;

    int next = pos + 1;
    if (line[next] == '{') {
        int end = next + 1;
        while (end < length && line[end] != '}') end++;
        return end < length ? end + 1 - pos : length - pos;
    }
    if (isalpha((unsigned char)line[next]) || line[next] == '_') {
        int end = next + 1;
        while (end < length &&
               (isalnum((unsigned char)line[end]) || line[end] == '_')) end++;
        return end - pos;
    }
    if (isdigit((unsigned char)line[next]) ||
        strchr("@*#?$!-", line[next]) != NULL) return 2;
    return 0;
}

/* The public context records that a quote spans lines; this character records
 * which of the shell's three quote forms opened it. Highlighting is
 * single-threaded and processes one fenced block at a time. */
static char shell_open_quote = '"';

static int shell_tokenise(const char *line, md_hl_context_t *ctx,
                          md_hl_span_t *spans, int max_spans) {
    int length = (int)strlen(line);
    int count = 0;
    int pos = 0;

    if (*ctx == MD_HL_CTX_STRING) {
        int end = shell_quote_end(line, 0, length, shell_open_quote);
        if (end < 0) {
            shell_emit(spans, &count, max_spans, 0, length, MD_HL_STRING);
            return count;
        }
        if (!shell_emit(spans, &count, max_spans, 0, end, MD_HL_STRING)) {
            return count;
        }
        *ctx = MD_HL_CTX_GROUND;
        pos = end;
    }

    while (pos < length && count < max_spans) {
        if (line[pos] == ' ' || line[pos] == '\t') {
            int start = pos;
            while (pos < length && (line[pos] == ' ' || line[pos] == '\t')) pos++;
            if (!shell_emit(spans, &count, max_spans, start, pos - start,
                            MD_HL_NORMAL)) break;
            continue;
        }

        if (line[pos] == '#' && shell_comment_start(line, pos)) {
            shell_emit(spans, &count, max_spans, pos, length - pos,
                       MD_HL_COMMENT);
            break;
        }

        if (line[pos] == '\'' || line[pos] == '"' || line[pos] == '`') {
            int start = pos;
            char quote = line[pos++];
            int end = shell_quote_end(line, pos, length, quote);
            if (end < 0) {
                shell_open_quote = quote;
                *ctx = MD_HL_CTX_STRING;
                shell_emit(spans, &count, max_spans, start, length - start,
                           MD_HL_STRING);
                break;
            }
            if (!shell_emit(spans, &count, max_spans, start, end - start,
                            MD_HL_STRING)) break;
            pos = end;
            continue;
        }

        if (line[pos] == '$') {
            int variable_len = shell_variable_len(line, pos, length);
            if (variable_len > 0) {
                if (!shell_emit(spans, &count, max_spans, pos, variable_len,
                                MD_HL_VARIABLE)) break;
                pos += variable_len;
                continue;
            }
        }

        if (isdigit((unsigned char)line[pos])) {
            int start = pos++;
            while (pos < length && isdigit((unsigned char)line[pos])) pos++;
            if (!shell_emit(spans, &count, max_spans, start, pos - start,
                            MD_HL_NUMBER)) break;
            continue;
        }

        if (isalpha((unsigned char)line[pos]) || line[pos] == '_') {
            int start = pos++;
            while (pos < length &&
                   (isalnum((unsigned char)line[pos]) || line[pos] == '_')) pos++;
            md_hl_token_t token = shell_keyword(line + start, pos - start)
                ? MD_HL_KEYWORD : MD_HL_NORMAL;
            if (!shell_emit(spans, &count, max_spans, start, pos - start,
                            token)) break;
            continue;
        }

        if (!shell_emit(spans, &count, max_spans, pos, 1, MD_HL_OPERATOR)) break;
        pos++;
    }

    return count;
}

static const char *shell_aliases[] = {
    "bash", "zsh", "ksh", "shell", NULL
};

const md_lang_t md_lang_shell = {
    .name = "sh",
    .aliases = shell_aliases,
    .tokenise = shell_tokenise,
};
