/*
 * libcbb test suite - assert-based.
 *
 * Each test converts a BBCode snippet and checks the rendered HTML exactly
 * matches the expected string. Run via ctest.
 *
 * The test executable is the single translation unit that defines
 * CBB_IMPLEMENTATION before including the header, so the implementation
 * is compiled inline with the tests (header-only consumption).
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

/* ----- inline text ------------------------------------------------------- */
static void test_text(void)
{
    check("text_plain", "Hello world", "Hello world");
    check("text_empty", "", "");
    check("text_null", NULL, "");
    check("text_escape", "<b>&\"'", "&lt;b&gt;&amp;&quot;&#x27;");
    check("text_amp", "A & B", "A &amp; B");
}

/* ----- simple inline tags ----------------------------------------------- */
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

/* ----- heading tags ----------------------------------------------------- */
static void test_headings(void)
{
    check("h1", "[h1]t[/h1]", "<h1>t</h1>");
    check("h2", "[h2]t[/h2]", "<h2>t</h2>");
    check("h3", "[h3]t[/h3]", "<h3>t</h3>");
    check("h4", "[h4]t[/h4]", "<h4>t</h4>");
    check("heading", "[heading]t[/heading]", "<h3>t</h3>");
}

/* ----- styled spans (color, size, font) --------------------------------- */
static void test_styled(void)
{
    check("color", "[color=red]x[/color]", "<span style=\"color:red;\">x</span>");
    check("size", "[size=20px]x[/size]", "<span style=\"font-size:20px;\">x</span>");
    check("font", "[font=monospace]x[/font]", "<span style=\"font-family:monospace;\">x</span>");
    check("color_escape", "[color=#ff0000]x[/color]", "<span style=\"color:#ff0000;\">x</span>");
}

/* ----- url / img / email / youtube -------------------------------------- */
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
    check("email", "[email=foo@bar.com]mail[/email]",
          "<a href=\"mailto:foo@bar.com\">mail</a>");
    check("youtube", "[youtube=tax4e4hBBZc][/youtube]",
          "<a href=\"https://www.youtube.com/watch?v=tax4e4hBBZc\">YouTube</a>");
    check("youtube_text", "[youtube=abc]watch[/youtube]",
          "<a href=\"https://www.youtube.com/watch?v=abc\">YouTube</a>watch");
}

/* ----- lists ------------------------------------------------------------ */
static void test_lists(void)
{
    check("list", "[list][*]a[*]b[*]c[/list]", "<ul><li>a</li><li>b</li><li>c</li></ul>");
    check("olist", "[olist][*]a[*]b[/olist]", "<ol><li>a</li><li>b</li></ol>");
    check("ol_alias", "[ol][*]a[/ol]", "<ol><li>a</li></ol>");
    check("ul_alias", "[ul][*]a[/ul]", "<ul><li>a</li></ul>");
    check("list_explicit_li", "[list][*]a[/li][*]b[/list]", "<ul><li>a</li><li>b</li></ul>");
}

/* ----- hr / line -------------------------------------------------------- */
static void test_hr(void)
{
    check("hr", "[hr][/hr]", "<hr>");
    check("line", "[line]", "<hr>");
}

/* ----- quote / spoiler -------------------------------------------------- */
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

/* ----- code / noparse --------------------------------------------------- */
static void test_raw(void)
{
    check("code_basic", "[code]int x = 1;[/code]", "<pre><code>int x = 1;</code></pre>");
    check("code_preserves_spaces", "[code]  spaces   stay  [/code]",
          "<pre><code>  spaces   stay  </code></pre>");
    check("code_inner_tags", "[code][b]bold[/b][/code]",
          "<pre><code><b>bold</b></code></pre>");
    check("noparse", "[noparse][b]x[/b][/noparse]", "[b]x[/b]");
    check("noparse_nested", "[noparse]a < b & c[/noparse]", "a &lt; b &amp; c");
}

/* ----- tables ----------------------------------------------------------- */
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

/* ----- unknown tags ----------------------------------------------------- */
static void test_unknown(void)
{
    check("unknown", "[foo]x[/foo]", "x");
    check("unknown_arg", "[foo=bar]x[/foo]", "x");
    check("unknown_mixed", "before [foo]inner[/foo] after", "before inner after");
}

/* ----- malformed input / EOF recovery ---------------------------------- */
static void test_malformed(void)
{
    /* Unclosed tag is auto-closed at EOF. */
    check("unclosed_b", "[b]x", "<b>x</b>");
    /* Stray close is ignored. */
    check("stray_close", "x[/b]", "x");
    /* Bad bracket falls back to text. */
    check("no_close_b", "[unterminated", "[unterminated");
    /* Unmatched close in middle of valid stream: stray [/i] is ignored. */
    check("nested_close", "[b]a[/i]b[/b]", "<b>ab</b>");
    /* Empty input. */
    check("just_kwarg", "[color=red][/color]", "<span style=\"color:red;\"></span>");
}

/* ----- Steam sample (the user-provided one) ----------------------------- */
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
    /* Smoke test: confirm key substrings are present. We don't pin the
     * whitespace exactly because the BBCode uses blank-line separators that
     * tokenize into TEXT with surrounding whitespace. */
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
    assert(strstr(out, "<tr><th>Name</th><th>Age</th></tr>"));
    assert(strstr(out, "<tr><td>John</td><td>65</td></tr>"));
    cbb_free(out);
    printf("ok   steam_sample\n");
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
    printf("All tests passed.\n");
    return 0;
}