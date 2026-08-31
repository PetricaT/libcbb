/*
 * libcbb - tiny BBCode-to-HTML microlibrary
 *
 * MIT License
 * Copyright (c) 2026 Petrica
 *
 * A single-header, zero-dependency C11 BBCode renderer using the stb-style
 * amalgamated distribution pattern. Two-function API: cbb_to_html(input)
 * and cbb_free(s). See README.md for the supported tag set.
 *
 * ----- How to use -----
 *
 * Option A - header-only (recommended for small projects):
 *
 *   In EXACTLY ONE .c file in your project:
 *
 *     #define CBB_IMPLEMENTATION
 *     #include "libcbb.h"
 *
 *   In every other .c file that calls cbb_to_html/cbb_free, just:
 *
 *     #include "libcbb.h"
 *
 *   No .c file to compile separately. No library to link.
 *
 * Option B - compile the implementation separately (faster incremental builds):
 *
 *   Copy include/libcbb.h into your project. Create a libcbb.c that does:
 *
 *     #define CBB_IMPLEMENTATION
 *     #include "libcbb.h"
 *
 *   Compile libcbb.c once and link the resulting object. Other translation
 *   units include "libcbb.h" without defining CBB_IMPLEMENTATION.
 *
 * Option C - CMake subdirectory:
 *
 *   add_subdirectory(libcbb)
 *   target_link_libraries(your_target PRIVATE libcbb)
 *   # In your sources: #include "libcbb.h"
 *
 * The CMake target builds the test executable only. To consume libcbb from
 * your own CMake project, just include the header (Option A or B above) - the
 * CMake target exists so this repo's own tests can be built and run via ctest.
 */

#ifndef LIBCBB_H
#define LIBCBB_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Convert a BBCode string to HTML.
 *
 * - `input` must be a NUL-terminated UTF-8 (or ASCII) string. NULL is treated
 *   as an empty string.
 * - Returns a newly allocated, NUL-terminated HTML string on success, or NULL
 *   if allocation fails.
 * - The returned string must be released with `cbb_free`.
 *
 * Supported tags: b, i, u, s, strike, color, size, font, url, img, quote,
 * code, list, olist, *, hr, center, right, spoiler, noparse, heading, h1-h4,
 * sub, sup, youtube, email, table, tr, th, td, line. Unknown tags are
 * dropped; their content passes through as escaped text. [code] and
 * [noparse] content is escaped but otherwise emitted verbatim. URLs that
 * begin with `javascript:` are stripped from href/src attributes.
 */
char *cbb_to_html(const char *input);

/* Free a string returned by `cbb_to_html`. NULL is a no-op. */
void cbb_free(char *s);

#ifdef __cplusplus
}
#endif

/* ========================================================================
 * Implementation
 *
 * Everything below this point is compiled only when CBB_IMPLEMENTATION is
 * defined. Define it in exactly ONE translation unit (typically a .c file)
 * before including this header. Other translation units include the header
 * without defining CBB_IMPLEMENTATION - they get the declarations only.
 * ======================================================================== */
#ifdef CBB_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>

typedef struct cbb_tag_def {
    const char *name;       /* lowercase BBCode tag name */
    const char *html_open;  /* opening template; %s becomes the arg when has_arg */
    const char *html_close; /* closing markup; may be empty */
    int has_arg;            /* 1 if [name=arg] is supported */
} cbb_tag_def;

/* The single tag table. Order does not matter; lookup is by name. */
static const cbb_tag_def cbb_tags[] = {
    {"b",       "<b>",                                          "</b>",          0},
    {"i",       "<i>",                                          "</i>",          0},
    {"u",       "<u>",                                          "</u>",          0},
    {"s",       "<s>",                                          "</s>",          0},
    {"strike",  "<s>",                                          "</s>",          0},
    {"color",   "<span style=\"color:%s;\">",                   "</span>",       1},
    {"size",    "<span style=\"font-size:%s;\">",                "</span>",       1},
    {"font",    "<span style=\"font-family:%s;\">",             "</span>",       1},
    {"url",     "<a href=\"%s\">",                               "</a>",          1},
    {"img",     "<img src=\"%s\" alt=\"\">",                    "",              1},
    {"quote",   "<blockquote>",                                 "</blockquote>", 0},
    {"code",    "<pre><code>",                                  "</code></pre>", 0},
    {"list",    "<ul>",                                         "</ul>",         0},
    {"olist",   "<ol>",                                         "</ol>",         0},
    {"ol",      "<ol>",                                         "</ol>",         0},
    {"ul",      "<ul>",                                         "</ul>",         0},
    {"*",       "<li>",                                         "</li>",         0},
    {"li",      "<li>",                                         "</li>",         0},
    {"hr",      "<hr>",                                         "",              0},
    {"line",    "<hr>",                                         "",              0},
    {"center",  "<div style=\"text-align:center;\">",           "</div>",        0},
    {"right",   "<div style=\"text-align:right;\">",            "</div>",        0},
    {"spoiler", "<details><summary>Spoiler</summary>",          "</details>",    0},
    {"noparse", "",                                             "",              0},
    {"heading", "<h3>",                                         "</h3>",         0},
    {"h1",      "<h1>",                                         "</h1>",         0},
    {"h2",      "<h2>",                                         "</h2>",         0},
    {"h3",      "<h3>",                                         "</h3>",         0},
    {"h4",      "<h4>",                                         "</h4>",         0},
    {"sub",     "<sub>",                                        "</sub>",        0},
    {"sup",     "<sup>",                                        "</sup>",        0},
    {"youtube", "<a href=\"https://www.youtube.com/watch?v=%s\">YouTube</a>", "", 1},
    {"email",   "<a href=\"mailto:%s\">",                       "</a>",          1},
    {"table",   "<table>",                                      "</table>",      0},
    {"tr",      "<tr>",                                         "</tr>",         0},
    {"th",      "<th>",                                         "</th>",         0},
    {"td",      "<td>",                                         "</td>",         0},
};
#define CBB_N_TAGS ((int)(sizeof cbb_tags / sizeof cbb_tags[0]))

static int cbb_lo(int c) { return (c >= 'A' && c <= 'Z') ? c + 32 : c; }

static int cbb_eq(const char *a, int alen, const char *b) {
    int blen = (int)strlen(b);
    if (blen != alen) return 0;
    for (int i = 0; i < alen; ++i)
        if (cbb_lo((unsigned char)a[i]) != cbb_lo((unsigned char)b[i])) return 0;
    return 1;
}

static const cbb_tag_def *cbb_lookup(const char *name, int nlen) {
    for (int i = 0; i < CBB_N_TAGS; ++i)
        if (cbb_eq(name, nlen, cbb_tags[i].name)) return &cbb_tags[i];
    return NULL;
}

/* --------- growable output buffer --------- */

typedef struct { char *data; size_t len, cap; int oom; } cbb_buf;

static void cbb_grow(cbb_buf *b, size_t need) {
    if (b->oom || b->cap - b->len > need) return;
    size_t cap = b->cap ? b->cap : 64;
    while (cap - b->len <= need) {
        if (cap > (size_t)-1 / 2) { b->oom = 1; return; }
        cap *= 2;
    }
    char *p = (char *)realloc(b->data, cap);
    if (!p) { b->oom = 1; return; }
    b->data = p; b->cap = cap;
}

static void cbb_putc(cbb_buf *b, char c) {
    cbb_grow(b, 1);
    if (b->oom) return;
    b->data[b->len++] = c;
    b->data[b->len] = '\0';
}

static void cbb_putsn(cbb_buf *b, const char *s, size_t n) {
    cbb_grow(b, n);
    if (b->oom) return;
    memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = '\0';
}

static void cbb_puts(cbb_buf *b, const char *s) { cbb_putsn(b, s, strlen(s)); }

/* HTML-escape `s` (length n) into `b`. Used for both element text and
 * attribute values - the entity set is the same for both. */
static void cbb_esc(cbb_buf *b, const char *s, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        unsigned char c = (unsigned char)s[i];
        switch (c) {
        case '<':  cbb_puts(b, "&lt;");   break;
        case '>':  cbb_puts(b, "&gt;");   break;
        case '&':  cbb_puts(b, "&amp;");  break;
        case '"':  cbb_puts(b, "&quot;"); break;
        case '\'': cbb_puts(b, "&#x27;"); break;
        default:   cbb_putc(b, (char)c);  break;
        }
    }
}

/* Returns 0 if `raw` has a safe URL scheme (or no scheme / relative path),
 * -1 otherwise. Safe schemes: http, https, mailto, ftp, ftps. */
static int cbb_url_ok(const char *raw) {
    const char *colon = NULL;
    for (const char *p = raw; *p; ++p) {
        int ch = (unsigned char)*p;
        if (ch == ':') { colon = p; break; }
        if (ch == '/' || ch == '?' || ch == '#') break;
        if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
              (ch >= '0' && ch <= '9') || ch == '+' || ch == '-' || ch == '.'))
            break;
    }
    if (!colon) return 0;
    size_t slen = (size_t)(colon - raw);
    if (slen > 15) return -1;
    char s[16];
    for (size_t i = 0; i < slen; ++i) s[i] = (char)cbb_lo((unsigned char)raw[i]);
    s[slen] = '\0';
    static const char *safe[] = {"http", "https", "mailto", "ftp", "ftps", NULL};
    for (int i = 0; safe[i]; ++i) if (strcmp(s, safe[i]) == 0) return 0;
    return -1;
}

/* --------- tokenizer --------- */

typedef enum { T_TEXT, T_OPEN, T_CLOSE } cbb_kind;

typedef struct cbb_tok {
    cbb_kind kind;
    const char *name;       /* points into input */
    int name_len;
    const char *arg;        /* NULL if no argument */
    int arg_len;
    /* OPEN/CLOSE only: full original "[...]" substring, used to emit
     * verbatim when we are inside [noparse]. */
    const char *raw;
    int raw_len;
    /* TEXT only: */
    const char *text;
    int text_len;
} cbb_tok;

static int cbb_tok_(const char *in, cbb_tok *o, int cap) {
    int n = 0;
    const char *p = in, *ts = p;
    while (*p && n < cap) {
        if (*p != '[') { ++p; continue; }
        if (p > ts) {
            o[n].kind = T_TEXT; o[n].text = ts;
            o[n].text_len = (int)(p - ts);
            ++n;
            if (n >= cap) return n;
        }
        const char *rb = strchr(p, ']');
        if (!rb) { ++p; continue; }  /* unterminated [ -> literal text */
        const char *q = p + 1;
        int cl = (*q == '/');
        if (cl) ++q;
        const char *ns = q;
        while (q < rb && (((*q | 32) >= 'a' && (*q | 32) <= 'z') ||
                          (*q >= '0' && *q <= '9') || *q == '_' || *q == '*'))
            ++q;
        if (q == ns) { ts = p + 1; ++p; continue; } /* empty name */
        cbb_tok *t = &o[n++];
        t->name = ns; t->name_len = (int)(q - ns);
        t->arg = NULL; t->arg_len = 0;
        if (!cl && q < rb && *q == '=') { ++q; t->arg = q; t->arg_len = (int)(rb - q); }
        t->kind = cl ? T_CLOSE : T_OPEN;
        t->raw = p; t->raw_len = (int)(rb - p) + 1;
        p = rb + 1; ts = p;
    }
    if (*ts && n < cap) {
        o[n].kind = T_TEXT; o[n].text = ts;
        o[n].text_len = (int)strlen(ts);
        ++n;
    }
    return n;
}

/* --------- validation / output --------- */

/* Stack entry. tag == NULL means an unknown/noparse/code tag that has no
 * HTML markup to emit but still occupies a slot for [/name] matching. */
typedef struct cbb_st {
    const cbb_tag_def *tag;
    char *arg;  /* heap-allocated NUL-terminated arg (NULL if no arg) */
} cbb_st;

static void cbb_emit_open(cbb_buf *o, const cbb_st *e) {
    if (!e->tag || !e->tag->html_open[0]) return;
    const char *tm = e->tag->html_open;
    const char *pc = strchr(tm, '%');
    if (!pc || pc[1] != 's') { cbb_puts(o, tm); return; }
    cbb_putsn(o, tm, (size_t)(pc - tm));
    if (e->arg) cbb_esc(o, e->arg, strlen(e->arg));
    cbb_puts(o, pc + 2);
}

static void cbb_emit_close(cbb_buf *o, const cbb_st *e) {
    if (e->tag && e->tag->html_close[0]) cbb_puts(o, e->tag->html_close);
}

/* Find the topmost stack entry whose tag matches `name`, or -1. */
static int cbb_find(const cbb_st *st, int d, const char *n, int nl) {
    for (int i = d - 1; i >= 0; --i)
        if (st[i].tag && cbb_eq(n, nl, st[i].tag->name)) return i;
    return -1;
}

/* Close stack entries from `to` up to (depth-1) (inclusive), emit closes,
 * free their args, and update `d` to `to`. */
static void cbb_pop_to(cbb_buf *o, cbb_st *st, int *d, int to) {
    for (int j = *d - 1; j > to; --j) cbb_emit_close(o, &st[j]);
    cbb_emit_close(o, &st[to]);
    for (int j = to; j < *d; ++j) free(st[j].arg);
    *d = to;
}

/* Find the topmost <li> on the stack, or -1. */
static int cbb_find_li(const cbb_st *st, int d) {
    for (int i = d - 1; i >= 0; --i)
        if (st[i].tag && cbb_eq("*", 1, st[i].tag->name)) return i;
    return -1;
}

char *cbb_to_html(const char *input) {
    if (!input) input = "";

    size_t ilen = strlen(input);
    int cap = (int)(ilen / 2 + 16);
    cbb_tok *toks = (cbb_tok *)malloc(sizeof *toks * (size_t)cap);
    if (!toks) return NULL;
    int n = cbb_tok_(input, toks, cap);

    cbb_buf out = {0};
    cbb_st stack[64];
    int depth = 0, in_code = 0, noparse = 0;

    for (int i = 0; i < n; ++i) {
        const cbb_tok *t = &toks[i];
        if (t->kind == T_TEXT) { cbb_esc(&out, t->text, (size_t)t->text_len); continue; }
        const cbb_tag_def *def = cbb_lookup(t->name, t->name_len);

        /* Inside [noparse]: emit tags verbatim until [/noparse]. */
        if (noparse > 0) {
            if (t->kind == T_CLOSE && def && cbb_eq(t->name, t->name_len, "noparse")) {
                noparse--;
                if (depth > 0) --depth;
                continue;
            }
            cbb_esc(&out, t->raw, (size_t)t->raw_len);
            continue;
        }

        if (t->kind == T_OPEN) {
            /* [code] wraps in <pre><code>; inner tags still process.
             * [noparse] suppresses inner tag processing. */
            if (def && cbb_eq(t->name, t->name_len, "code")) {
                stack[depth].tag = def; stack[depth].arg = NULL; depth++;
                cbb_puts(&out, "<pre><code>"); in_code++;
                continue;
            }
            if (def && cbb_eq(t->name, t->name_len, "noparse")) {
                stack[depth].tag = NULL; stack[depth].arg = NULL; depth++;
                noparse++;
                continue;
            }
            /* [*] auto-closes any open <li>. */
            if (def && cbb_eq(t->name, t->name_len, "*")) {
                int li = cbb_find_li(stack, depth);
                if (li >= 0) cbb_pop_to(&out, stack, &depth, li);
            }
            /* Unknown tag: drop the markers, content passes through. */
            if (!def) {
                stack[depth].tag = NULL; stack[depth].arg = NULL; depth++;
                continue;
            }
            char *own = NULL;
            if (def->has_arg && t->arg) {
                own = (char *)malloc((size_t)t->arg_len + 1);
                if (!own) { out.oom = 1; break; }
                memcpy(own, t->arg, (size_t)t->arg_len);
                own[t->arg_len] = '\0';
                int url_tag = cbb_eq(t->name, t->name_len, "url")
                           || cbb_eq(t->name, t->name_len, "img")
                           || cbb_eq(t->name, t->name_len, "youtube")
                           || cbb_eq(t->name, t->name_len, "email");
                if (url_tag && cbb_url_ok(own) != 0) {
                    /* Dangerous scheme: drop the tag. */
                    free(own);
                    stack[depth].tag = NULL; stack[depth].arg = NULL; depth++;
                    continue;
                }
            }
            stack[depth].tag = def; stack[depth].arg = own; depth++;
            cbb_emit_open(&out, &stack[depth - 1]);
            continue;
        }

        /* CLOSE */
        if (def && cbb_eq(t->name, t->name_len, "noparse")) {
            if (depth > 0) --depth;
            continue;
        }
        if (def && cbb_eq(t->name, t->name_len, "code")) {
            if (in_code > 0) { cbb_puts(&out, "</code></pre>"); in_code--; }
            if (depth > 0) --depth;
            continue;
        }
        if (def && cbb_eq(t->name, t->name_len, "*")) {
            int li = cbb_find_li(stack, depth);
            if (li >= 0) cbb_pop_to(&out, stack, &depth, li);
            continue;
        }
        int idx = cbb_find(stack, depth, t->name, t->name_len);
        if (idx >= 0) cbb_pop_to(&out, stack, &depth, idx);
        /* unmatched close: silently ignored */
    }

    /* Auto-close any remaining open tags. */
    for (int j = depth - 1; j >= 0; --j) cbb_emit_close(&out, &stack[j]);
    for (int j = 0; j < depth; ++j) free(stack[j].arg);
    free(toks);

    if (out.oom) { free(out.data); return NULL; }
    if (!out.data) { /* empty input -> malloc(1) so cbb_free has something */
        out.data = (char *)malloc(1);
        if (!out.data) return NULL;
        out.data[0] = '\0';
    }
    return out.data;
}

void cbb_free(char *s) { free(s); }

#endif /* CBB_IMPLEMENTATION */

#endif /* LIBCBB_H */