/*
 * libcbb test suite - assert-based. Run via ctest.
 */

#define CBB_IMPLEMENTATION
#include "libcbb.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *render(const char *bbcode)
{
    char *out = cbb_to_html(bbcode);
    if (!out) {
        fprintf(stderr, "cbb_to_html returned NULL for: %s\n", bbcode);
        abort();
    }
    return out;
}

static void check(const char *name, const char *bbcode, const char *expected)
{
    char *got = render(bbcode);
    if (strcmp(got, expected) != 0) {
        fprintf(stderr, "FAIL %s\n  in:  %s\n  got:  %s\n  want: %s\n",
                name, bbcode, got, expected);
        cbb_free(got);
        exit(1);
    }
    cbb_free(got);
    printf("ok   %s\n", name);
}

/* ----- inline text ----- */
static void test_text(void)
{
    check("text_plain", "Hello world", "Hello world");
    check("text_empty", "", "");
    check("text_null", NULL, "");
    check("text_escape", "<b>&\"'", "&lt;b&gt;&amp;&quot;&#x27;");
    check("text_amp", "A & B", "A &amp; B");
}

/* ----- simple inline tags ----- */
static void test_inline(void)
{
    check("b", "[b]x[/b]", "<b>x</b>");
    check("i", "[i]x[/i]", "<i>x</i>");
    check("u", "[u]x[/u]", "<u>x</u>");
    check("s", "[s]x[/s]", "<s>x</s>");
    check("strike", "[strike]x[/strike]", "<s>x</s>");
    check("uppercase", "[B]x[/B]", "<b>x</b>");
    check("nested_bi", "[b][i]x[/i][/b]", "<b><i>x</i></b>");
    check("sub", "[sub]x[/sub]", "<sub>x</sub>");
    check("sup", "[sup]x[/sup]", "<sup>x</sup>");
}

/* ----- heading tags ----- */
static void test_headings(void)
{
    check("h1", "[h1]t[/h1]", "<h1>t</h1>");
    check("h2", "[h2]t[/h2]", "<h2>t</h2>");
    check("h3", "[h3]t[/h3]", "<h3>t</h3>");
    check("h4", "[h4]t[/h4]", "<h4>t</h4>");
    check("heading", "[heading]t[/heading]", "<h3>t</h3>");
}

/* ----- styled spans (color, size, font) ----- */
static void test_styled(void)
{
    check("color", "[color=red]x[/color]", "<span style=\"color:red;\">x</span>");
    check("size", "[size=20px]x[/size]", "<span style=\"font-size:20px;\">x</span>");
    check("font", "[font=monospace]x[/font]", "<span style=\"font-family:monospace;\">x</span>");
    check("color_escape", "[color=#ff0000]x[/color]", "<span style=\"color:#ff0000;\">x</span>");
    /* CSS meta-chars (; { } ( )) and quote/control chars must be rejected. */
    check("color_inject_semi", "[color=red;}body{background:url(x)]y[/color]", "y");
    check("color_inject_brace", "[color=red{color:blue}]y[/color]", "y");
    check("color_inject_paren", "[color=red(0)]y[/color]", "y");
    check("color_inject_space2", "[color=red  blue]y[/color]", "y");
    check("color_inject_tab", "[color=red\tblue]y[/color]", "y");
    check("size_inject_semi", "[size=12px;]y[/size]", "y");
    check("font_inject_brace", "[font=monospace;}x]y[/font]", "y");
    check("color_inject_dquote", "[color=red\"x]y[/color]", "y");
    check("color_inject_squote", "[color=red'x]y[/color]", "y");
    check("size_inject_dquote", "[size=12px\"]y[/size]", "y");
    check("font_inject_squote", "[font=mono']y[/font]", "y");
}

/* ----- url / img / email / youtube ----- */
static void test_links(void)
{
    check("url", "[url=https://example.com]link[/url]",
          "<a href=\"https://example.com\">link</a>");
    check("url_unsafe", "[url=javascript:alert(1)]x[/url]", "x");
    check("url_quote", "[url=https://a.com?x=\"y\"]x[/url]",
          "<a href=\"https://a.com?x=&quot;y&quot;\">x</a>");
    check("img", "[img=https://example.com/p.png]",
          "<img src=\"https://example.com/p.png\" alt=\"\">");
    check("img_unsafe", "[img=javascript:x]", "");
    /* [img]url[/img] form: inner text is the src. */
    check("img_inner_form", "[img]https://example.com/p.png[/img]",
          "<img src=\"https://example.com/p.png\" alt=\"\">");
    check("img_inner_unsafe", "[img]javascript:x[/img]", "");
    check("img_inner_tab", "[img]java\tscript:x[/img]", "");
    check("img_inner_space", "[img] javascript:x[/img]", "");
    /* URL sanitizer bypass via tab/whitespace (WHATWG strip). */
    check("url_tab_bypass", "[url=java\tscript:alert(1)]click[/url]", "click");
    check("url_space_bypass", "[url= javascript:alert(1)]click[/url]", "click");
    check("url_lf_bypass", "[url=java\nscript:alert(1)]click[/url]", "click");
    check("url_cr_bypass", "[url=java\rscript:alert(1)]click[/url]", "click");
    check("url_trailing_space", "[url=https://example.com  ]x[/url]", "x");
    check("url_leading_space", "[url=  https://example.com]x[/url]", "x");
    /* Long URL (over an older 256-byte fixed cap). */
    {
        char url_arg[400];
        size_t pos = 0;
        memcpy(url_arg + pos, "https://example.com/", 20); pos += 20;
        for (int i = 0; i < 280; ++i) url_arg[pos++] = 'a';
        url_arg[pos] = '\0';
        char in_bb[512];
        snprintf(in_bb, sizeof in_bb, "[url=%s]link[/url]", url_arg);
        char want[512];
        snprintf(want, sizeof want, "<a href=\"%s\">link</a>", url_arg);
        check("url_long_300", in_bb, want);
    }
    {
        char url_arg[400];
        size_t pos = 0;
        memcpy(url_arg + pos, "https://cdn.example.com/path/", 30); pos += 30;
        for (int i = 0; i < 280; ++i) url_arg[pos++] = 'p';
        url_arg[pos] = '\0';
        char in_bb[512];
        snprintf(in_bb, sizeof in_bb, "[img=%s]", url_arg);
        char want[512];
        snprintf(want, sizeof want, "<img src=\"%s\" alt=\"\">", url_arg);
        check("img_arg_long_300", in_bb, want);
    }
    /* BBCode markup inside [img]...[/img] is stripped, leaving only the
     * text content as src. */
    check("img_inner_bbcode_stripped",
          "[img][b]https://example.com/p.png[/b][/img]",
          "<img src=\"https://example.com/p.png\" alt=\"\">");
    check("email", "[email=foo@bar.com]mail[/email]",
          "<a href=\"mailto:foo@bar.com\">mail</a>");
    check("youtube", "[youtube=tax4e4hBBZc][/youtube]",
          "<a href=\"https://www.youtube.com/watch?v=tax4e4hBBZc\">YouTube</a>");
    check("youtube_text", "[youtube=abc]watch[/youtube]",
          "<a href=\"https://www.youtube.com/watch?v=abc\">YouTube</a>watch");
}

/* ----- lists ----- */
static void test_lists(void)
{
    check("list", "[list][*]a[*]b[*]c[/list]", "<ul><li>a</li><li>b</li><li>c</li></ul>");
    check("olist", "[olist][*]a[*]b[/olist]", "<ol><li>a</li><li>b</li></ol>");
    check("ol_alias", "[ol][*]a[/ol]", "<ol><li>a</li></ol>");
    check("ul_alias", "[ul][*]a[/ul]", "<ul><li>a</li></ul>");
    check("list_explicit_li", "[list][*]a[/li][*]b[/list]", "<ul><li>a</li><li>b</li></ul>");
}

/* ----- hr / line ----- */
static void test_hr(void)
{
    check("hr", "[hr][/hr]", "<hr>");
    check("line", "[line]", "<hr>");
}

/* ----- quote / spoiler ----- */
static void test_blocks(void)
{
    check("quote", "[quote]x[/quote]", "<blockquote>x</blockquote>");
    check("spoiler", "[spoiler]x[/spoiler]",
          "<details><summary>Spoiler</summary>x</details>");
    check("center", "[center]x[/center]",
          "<div style=\"text-align:center;\">x</div>");
    check("right", "[right]x[/right]",
          "<div style=\"text-align:right;\">x</div>");
}

/* ----- code / noparse ----- */
static void test_raw(void)
{
    check("code_basic", "[code]int x = 1;[/code]", "<pre><code>int x = 1;</code></pre>");
    check("code_preserves_spaces", "[code]  spaces   stay  [/code]",
          "<pre><code>  spaces   stay  </code></pre>");
    /* [code] implies noparse semantics: inner tags are NOT processed. */
    check("code_inner_tags", "[code][b]bold[/b][/code]",
          "<pre><code>[b]bold[/b]</code></pre>");
    check("code_inner_bold_rendered", "[code]a < b & c[/code]",
          "<pre><code>a &lt; b &amp; c</code></pre>");
    check("noparse", "[noparse][b]x[/b][/noparse]", "[b]x[/b]");
    check("noparse_nested", "[noparse]a < b & c[/noparse]", "a &lt; b &amp; c");
    /* Nested [code][code]: inner [code] is shown as literal text. */
    check("code_nested", "[code][code]x[/code][/code]",
          "<pre><code>[code]x</code></pre>");
    /* Cross-close: inner [/noparse] inside [code] is emitted verbatim. */
    check("code_inner_noparse_close_verbatim",
          "[code]a[/noparse]b[/code]",
          "<pre><code>a[/noparse]b</code></pre>");
    check("noparse_inner_code_close_verbatim",
          "[noparse]a[/code]b[/noparse]",
          "a[/code]b");
    /* [code]outer[code]inner[/code]tail[/code]: first [/code] closes the
     * block, 'tail' is plain text, final stray [/code] is dropped. */
    check("code_nested_no_leak",
          "[code]outer[code]inner[/code]tail[/code]",
          "<pre><code>outer[code]inner</code></pre>tail");
    /* Stray [/code] or [/noparse] when noparse==0 is silently ignored
     * (does not pop whatever is on top of the stack). */
    check("stray_code_in_b", "[b]hello[/code]world[/b]", "<b>helloworld</b>");
    check("stray_noparse_in_b", "[b]hello[/noparse]world[/b]", "<b>helloworld</b>");
    check("stray_code_in_nested", "[i][b]x[/code]y[/b][/i]", "<i><b>xy</b></i>");
    check("stray_noparse_in_nested", "[i][b]x[/noparse]y[/b][/i]", "<i><b>xy</b></i>");
}

/* ----- tables ----- */
static void test_tables(void)
{
    check("table_simple", "[table][tr][th]A[/th][/tr][/table]",
          "<table><tr><th>A</th></tr></table>");
    check("table_full",
          "[table]"
          "[tr][th]N[/th][th]A[/th][/tr]"
          "[tr][td]1[/td][td]2[/td][/tr]"
          "[/table]",
          "<table>"
          "<tr><th>N</th><th>A</th></tr>"
          "<tr><td>1</td><td>2</td></tr>"
          "</table>");
}

/* ----- unknown tags ----- */
static void test_unknown(void)
{
    check("unknown", "[foo]x[/foo]", "x");
    check("unknown_arg", "[foo=bar]x[/foo]", "x");
    check("unknown_mixed", "before [foo]inner[/foo] after", "before inner after");
}

/* ----- malformed input / EOF recovery ----- */
static void test_malformed(void)
{
    check("unclosed_b", "[b]x", "<b>x</b>");
    check("stray_close", "x[/b]", "x");
    check("no_close_b", "[unterminated", "[unterminated");
    check("nested_close", "[b]a[/i]b[/b]", "<b>ab</b>");
    check("just_kwarg", "[color=red][/color]", "<span style=\"color:red;\"></span>");
}

/* ----- Steam sample (the user-provided one) ----- */
static void test_steam_sample(void)
{
    const char *in = "[h1] Header text [/h1]\n"
                     "[h2] Header text [/h2]\n"
                     "[h3] Header text [/h3]\n"
                     "[b] Bold text [/b]\n"
                     "[u] Underlined text [/u]\n"
                     "[i] Italic text [/i]\n"
                     "[strike] Strikethrough text [/strike]\n"
                     "[spoiler] Spoiler text [/spoiler]\n"
                     "[noparse] Doesn't parse [b]tags[/b] [/noparse]\n"
                     "[hr][/hr]\n"
                     "[url=store.steampowered.com] Website link [/url]\n"
                     "https://www.youtube.com/watch?v=tax4e4hBBZc\n"
                     "https://store.steampowered.com/app/323190/\n"
                     "https://steamcommunity.com/sharedfiles/filedetails/?id=157328145\n"
                     "[list]\n    [*]Bulleted list\n    [*]Bulleted list\n    [*]Bulleted list\n[/list]\n"
                     "[olist]\n    [*]Ordered list\n    [*]Ordered list\n    [*]Ordered list\n[/olist]\n"
                     "[quote=author] Quoted text [/quote]\n"
                     "[code] Fixed-width font, preserves spaces [/code]\n"
                     "[table]\n"
                     "    [tr]\n        [th]Name[/th]\n        [th]Age[/th]\n    [/tr]\n"
                     "    [tr]\n        [td]John[/td]\n        [td]65[/td]\n    [/tr]\n"
                     "    [tr]\n        [td]Gitte[/td]\n        [td]40[/td]\n    [/tr]\n"
                     "    [tr]\n        [td]Sussie[/td]\n        [td]19[/td]\n    [/tr]\n"
                     "[/table]\n";
    /* Confirm key substrings. Whitespace is not pinned because the BBCode
     * uses blank-line separators that tokenize into TEXT with surrounding
     * whitespace. */
    char *out = render(in);
    assert(strstr(out, "<h1> Header text </h1>"));
    assert(strstr(out, "<h2> Header text </h2>"));
    assert(strstr(out, "<h3> Header text </h3>"));
    assert(strstr(out, "<b> Bold text </b>"));
    assert(strstr(out, "<u> Underlined text </u>"));
    assert(strstr(out, "<i> Italic text </i>"));
    assert(strstr(out, "<s> Strikethrough text </s>"));
    assert(strstr(out, "<details><summary>Spoiler</summary> Spoiler text </details>"));
    assert(strstr(out, "Doesn&#x27;t parse [b]tags[/b]"));
    assert(strstr(out, "<hr>"));
    assert(strstr(out, "<a href=\"store.steampowered.com\"> Website link </a>"));
    assert(strstr(out, "https://www.youtube.com/watch?v=tax4e4hBBZc"));
    assert(strstr(out, "<ul>"));
    assert(strstr(out, "<ol>"));
    assert(strstr(out, "<blockquote>"));
    assert(strstr(out, " Quoted text "));
    assert(strstr(out, "<pre><code> Fixed-width font, preserves spaces </code></pre>"));
    assert(strstr(out, "<table>"));
    /* Table rows have whitespace between <tr> and <th>/<td> because the
     * input is indented for readability - that whitespace becomes literal
     * text. Assert on the cells, which are contiguous regardless of
     * indentation. */
    assert(strstr(out, "<th>Name</th>"));
    assert(strstr(out, "<th>Age</th>"));
    assert(strstr(out, "<td>John</td>"));
    assert(strstr(out, "<td>65</td>"));
    cbb_free(out);
    printf("ok   steam_sample\n");
}

/* ----- deep nesting (stack grows on demand) ----- */
static void test_deep_nesting(void)
{
    size_t cap = 16 * 1024;
    char *in = (char *)malloc(cap);
    if (!in) { fprintf(stderr, "OOM\n"); exit(1); }
    char *p = in;
    for (int i = 0; i < 300; ++i) { memcpy(p, "[i]", 3); p += 3; }
    *p = '\0';
    char *out = render(in);
    int opens = 0;
    for (const char *q = out; (q = strstr(q, "<i>")) != NULL; q += 3) opens++;
    if (opens != 300) {
        fprintf(stderr, "FAIL deep_nesting: expected 300 <i> opens, got %d\n  out: %.200s...\n",
                opens, out);
        cbb_free(out); free(in); exit(1);
    }
    cbb_free(out);
    free(in);
    printf("ok   deep_nesting\n");
}

int main(void)
{
    test_text();
    test_inline();
    test_headings();
    test_styled();
    test_links();
    test_lists();
    test_hr();
    test_blocks();
    test_raw();
    test_tables();
    test_unknown();
    test_malformed();
    test_steam_sample();
    test_deep_nesting();
    printf("All tests passed.\n");
    return 0;
}