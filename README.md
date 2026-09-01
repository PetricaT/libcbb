# libcbb - (edoCBB)

A tiny BBCode-to-HTML microlibrary written in pure C11 with zero dependencies.
Distributed as an amalgamated single header (`libcbb.h`) using the
[stb-style](https://github.com/nothings/stb) `CBB_IMPLEMENTATION` pattern.
One file to vendor, no library to link, no build system required.

## Usage

Pick one of the three options below. The first one is the recommended default
for almost everyone.

### Option A - Header-only (recommended)

Drop `include/libcbb.h` into your project. In **exactly one** `.c` file:

```c
#define CBB_IMPLEMENTATION
#include "libcbb.h"
```

In every other `.c` file that calls `cbb_to_html` / `cbb_free`, just:

```c
#include "libcbb.h"
```

That's it. No `.c` file of your own to write, no library to link, no CMake.
Compile each of your translation units with `-std=c11` and you're done.

### Option B - Compiled from one .c (smaller incremental builds)

Same header, but instead of compiling the implementation inline in one of
your own `.c` files, create a `libcbb.c`:

```c
#define CBB_IMPLEMENTATION
#include "libcbb.h"
```

Compile `libcbb.c` once to `libcbb.o`, link it with your app, and have every
other `.c` file just `#include "libcbb.h"` without the macro. Use this when
you want shorter per-TU compile times. The shipped `src/libcbb.c` is exactly
this two-line shim.

### Option C - CMake subdirectory (only for running this repo's tests)

```cmake
add_subdirectory(libcbb)
target_link_libraries(your_target PRIVATE libcbb)
```

The CMake `libcbb` target is an INTERFACE library that just exposes the
header path; it does not compile anything itself. Your code still has to
include the header with `CBB_IMPLEMENTATION` defined in exactly one TU
(Options A or B above).

To also build and run this repo's tests:

```bash
cmake -S projects/libcbb -B projects/libcbb/build -G Ninja \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build projects/libcbb/build
ctest --test-dir projects/libcbb/build --output-on-failure
```

`-std=c11`, `-Wall -Wextra -Wpedantic`, no GNU extensions.

## API

```c
#include "libcbb.h"

/* Convert a NUL-terminated BBCode string to HTML. Returns a newly allocated
 * NUL-terminated string, or NULL on allocation failure. The caller releases
 * it with cbb_free(). */
char *cbb_to_html(const char *input);

/* Free a string returned by cbb_to_html. NULL is a no-op. */
void  cbb_free(char *s);
```

That's the entire API.

## Supported tags

A single hardcoded table drives rendering. Unknown tags are dropped (their
content passes through as escaped text). `[code]` escapes but otherwise
emits content verbatim. `[noparse]` suppresses inner tag processing.

| Input                          | Output                                              |
|--------------------------------|-----------------------------------------------------|
| `[b]x[/b]`                     | `<b>x</b>`                                          |
| `[i]x[/i]`                     | `<i>x</i>`                                          |
| `[u]x[/u]`                     | `<u>x</u>`                                          |
| `[s]x[/s]`, `[strike]x[/strike]`| `<s>x</s>`                                         |
| `[color=red]x[/color]`         | `<span style="color:red;">x</span>`                 |
| `[size=20px]x[/size]`          | `<span style="font-size:20px;">x</span>`            |
| `[font=mono]x[/font]`          | `<span style="font-family:mono;">x</span>`          |
| `[url=...]link[/url]`          | `<a href="...">link</a>`                            |
| `[img=src]`                    | `<img src="src" alt="">`                            |
| `[youtube=ID]...[/youtube]`    | `<a href="https://www.youtube.com/watch?v=ID">YouTube</a>` |
| `[email=a@b]text[/email]`      | `<a href="mailto:a@b">text</a>`                     |
| `[quote]x[/quote]`             | `<blockquote>x</blockquote>`                        |
| `[code]x[/code]`               | `<pre><code>x</code></pre>`                         |
| `[list][*]a[*]b[/list]`        | `<ul><li>a</li><li>b</li></ul>`                     |
| `[olist][*]a[*]b[/olist]`      | `<ol><li>a</li><li>b</li></ol>`                     |
| `[*]`, `[li]`                  | `<li>...</li>` (auto-closed when another opens)     |
| `[hr]`, `[line]`               | `<hr>`                                              |
| `[center]x[/center]`           | `<div style="text-align:center;">x</div>`           |
| `[right]x[/right]`             | `<div style="text-align:right;">x</div>`            |
| `[spoiler]x[/spoiler]`         | `<details><summary>Spoiler</summary>x</details>`    |
| `[noparse]x[/noparse]`         | `x` (inner tags not processed, content escaped)     |
| `[heading]x[/heading]`         | `<h3>x</h3>`                                        |
| `[h1]`..`[h4]`                 | `<h1>`..`<h4>`                                      |
| `[sub]x[/sub]`                 | `<sub>x</sub>`                                      |
| `[sup]x[/sup]`                 | `<sup>x</sup>`                                      |
| `[table][tr][th]x[/th][/tr][/table]` | `<table><tr><th>x</th></tr></table>`         |

Aliases: `ol`/`ul` for `olist`/`list`; `li` for `*`; `line` for `hr`. Tag names
are case-insensitive.
