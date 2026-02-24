# AGENTS.md

Instructions for AI assistants and agents working with code in this
repository.

## Overview

GRASS is a large, multi-language geospatial processing engine. The codebase
is primarily C (core libraries and tools) and Python (high-level API, scripts,
GUI, tests). Tools follow a `<type>.<name>` naming convention: `r.*` (raster),
`v.*` (vector), `g.*` (general), `d.*` (display), `i.*` (imagery), `t.*`
(temporal), `r3.*` (3D raster).

## Build

On the `main` branch, Autotools and CMake are equivalent. Older release
branches support Autotools only, and full documentation builds (MkDocs) also
require Autotools. See `INSTALL.md` for full build instructions including
dependencies and configuration options.

**Autotools:**

```bash
CFLAGS="-g -Wall" ./configure  # configure with debug flags
make  # compile everything
make install  # install
make libs  # compile libraries only
```

**CMake:**

```bash
cmake -B build
cmake --build build
cmake --install build
```

**Compile a single tool** (after libraries are built):

```bash
cd raster/r.slope.aspect
make
```

Build outputs go to `bin.$ARCH/` and `dist.$ARCH/` directories.

## Running Tests

Tests require a built and installed GRASS with the `grass` binary on `PATH`.
If running from a local build (not a system install), add the binary directory
to PATH first:

```bash
export PATH="$(pwd)/bin.$(uname -m)-pc-linux-gnu:${PATH}"
```

Before running pytest, set the required environment variables:

```bash
export PYTHONPATH="$(grass --config python_path):${PYTHONPATH}"
export LD_LIBRARY_PATH="$(grass --config path)/lib:${LD_LIBRARY_PATH}"
```

For coverage reporting, two additional variables are needed (used by
`.coveragerc` to map paths):

```bash
export INITIAL_GISBASE="$(grass --config path)"
export INITIAL_PWD="${PWD}"
```

**Run tests for a specific tool:**

```bash
pytest raster/r.slope.aspect/tests/
```

**Run a single test file:**

```bash
pytest raster/r.slope.aspect/tests/r_slope_aspect_test.py
```

**Run gunittest-style tests** (legacy `testsuite/` directories; currently the
only way to use larger datasets like the `nc_spm` sample dataset):

```bash
python -m grass.gunittest.main --grassdata /path/to/grassdata --location location --location-type xyz
```

Test files follow two patterns:

- `*/tests/*_test.py` or `*/tests/test_*.py` — pytest style (preferred for new tests)
- `*/testsuite/` directories — gunittest style (slightly less preferred, but
  required when tests need large external datasets)

pytest configuration is in `pyproject.toml`. pytest tests require a running
GRASS session with a project/mapset; see
`raster/r.slope.aspect/tests/conftest.py` for a representative session
fixture. For new pytest tests, use `grass.tools` with `Tools`: the `Tools`
object needs `session=session` (or `env=session.env`) at creation, but
individual tool calls do not need `env`. When using `grass.script` in
pytest, every call (`gs.run_command()`, `gs.parse_command()`, etc.) must
pass `env=session.env` explicitly. gunittest-style tests (`testsuite/`) run
in an existing GRASS session and need neither `Tools` setup nor `env`
passing.

## Linting and Formatting

See `doc/development/style_guide.md` for full formatting and style rules.
Quick reference:

```bash
pre-commit run --all-files  # run all checks
```

## Architecture

### Core Libraries (`lib/`)

C libraries linked by tools. Key ones:

- `lib/gis/` — coordinate systems, map management, environment (`libgis`)
- `lib/raster/` — raster I/O
- `lib/vector/` — vector topology and I/O
- `lib/db/` — database drivers (SQLite, PostgreSQL, etc.)
- `lib/imagery/` — imagery/classification routines
- `lib/temporal/` — time series framework support

### Python API (`python/grass/`)

Four primary packages:

- `grass.script` — procedural interface; wraps GRASS tools via `run_command`,
  `parse_command`, `read_command`, etc. Import as `import grass.script as gs`.
  Use this in `grass.script` itself and code that `grass.script` depends on.
- `grass.tools` — preferred interface for new code and new tests; provides a
  `Tools` class with direct Python attribute access to GRASS tools (e.g.,
  `tools.r_slope_aspect(...)`). Most existing code uses `grass.script`
  (`gs.run_command()` etc.), so expect to see both. Do not use `grass.tools`
  in `grass.script` or anything `grass.script` depends on (circular
  dependency). Not to be confused with the object-oriented API of
  `grass.pygrass`.
- `grass.pygrass` — object-oriented API; provides `Raster`, `Vector` classes
  and a lower-level module interface
- `grass.temporal` — space-time dataset (STRDS/STVDS) management. Import as
  `import grass.temporal as tgis`.

### Tool Structure

Each tool (e.g., `raster/r.slope.aspect/`) contains:

- `main.c` (or `<name>.py` for Python scripts) — entry point
- `Makefile` — follows a standard template; see
  `raster/r.slope.aspect/Makefile` (C tool) and `scripts/r.mask/Makefile`
  (Python script) for examples
- `CMakeLists.txt` — CMake build definition; see
  `raster/r.colors.out/CMakeLists.txt` for an example
- `<name>.md` — documentation source (no header/footer; Markdown rendered by
  MkDocs)
- `<name>.html` — supports a legacy documentation system; uses HTML markup
  for formatting but is not a complete HTML document (only the translated
  Markdown into corresponding HTML tags). Must be committed alongside the
  `.md` file.
- `tests/` — pytest tests
- `testsuite/` — gunittest tests (legacy)

### Python Scripts (`scripts/`)

Python-based tools (e.g., `scripts/r.mask/`) use `grass.script` and
`g.parser` for option parsing. They follow the same tool structure as C tools.

### GUI (`gui/wxpython/`)

wxPython desktop application. The GUI uses tools and libraries (Python and
C) to do geospatial and data work, so anything available in the GUI can
also be done with tools, the command line, or the Python and C APIs. Other
code should not depend on the GUI at runtime or during compilation, except
for unavoidable cases like starting the GUI from the command line. For
classes and code that inherit from wxPython classes or use their interface,
wxPython conventions are used rather than standard GRASS Python conventions.

## Key Coding Conventions

For full conventions, see `doc/development/style_guide.md`. The following
highlights rules that are especially important or easy for AI agents to
miss.

### Comments (all languages)

- No decorative comment banners or dividers (e.g., `# ---- Section ----`).
- No double space after a sentence-ending period.
- Write full sentences with proper capitalization, not stubs or all-lowercase
  fragments.
- Do not restate what the code already says; comment on intent, rationale,
  or non-obvious behavior.
- Strongly prefer plain ASCII for comments; use Unicode only when necessary.
  For Python docstrings and Doxygen comments, plain ASCII is still preferred
  but Unicode is more acceptable.
- Inline comments in code examples must be separated from code by exactly
  two spaces (e.g., `x = 1  # comment`).

### Tests

- Module-level globals in test files do not need a leading underscore; test
  modules are not public APIs.

### Python

- Import `grass.script` as `gs` (enforced by ruff `ICN001` rule)
- Import `grass.temporal` as `tgis`
- Use `str.format()` for translatable user messages (not f-strings):
  `gs.warning(_("Map <{}> not found.").format(name))`
- Use f-strings for non-translatable strings
- In tool code (not Python libraries): use `gs.fatal()` to exit with error,
  `gs.warning()` for warnings, `gs.message()` for informational output.
  Use `print()` only for text data output (e.g., results the user pipes
  elsewhere); never for messages to the user.
- Data is organized in **projects** (formerly called "locations") and mapsets

### C

- Use `G_malloc()`, `G_calloc()`, `G_free()` instead of standard C equivalents
- Use `G_fatal_error()`, `G_warning()`, `G_message()` for output
- Use `EXIT_SUCCESS`/`EXIT_FAILURE` return values from `main()`
- Include order: system headers, then non-core system libs, then GRASS headers
  (`grass/gis.h`, etc.), then local headers; groups alphabetically sorted,
  separated by blank lines
- Snake case for function names (GNU naming convention)

### Documentation (tool doc files)

Each tool must have both `<tool>.md` and `<tool>.html` committed to the
repository. The `.md` is the source of truth; the `.html` provides basic HTML
access and must be kept in sync. See `doc/development/style_guide.md` for
full markup conventions.

Required sections in the `.md`: `## DESCRIPTION`, `## SEE ALSO`, `## AUTHORS`
Suggested: `## NOTES`, `## EXAMPLES`

Markup conventions:

- Tool names in italic: `*r.slope.aspect*`
- Flags and parameter names in bold: `**-f**`, `**input**`
- Values, paths, commands in backticks
- `SEE ALSO` section alphabetized

### Tool Interface

Tools must use the GRASS parser for parameters. Use standard options
(`G_OPT_R_INPUT`, `G_OPT_V_OUTPUT`, etc.) instead of defining common
parameters from scratch. The full list of standard options is generated from
`lib/gis/parser_standard_options.c`. Prefer a `format` option over multiple exclusive
flags for output format selection. Output must always go to the current
mapset. Tools must not overwrite existing maps unless the user provides
`--overwrite`.

Tool name prefixes: `r.` (raster), `v.` (vector), `g.` (general), `d.`
(display), `i.` (imagery), `t.` (temporal), `r3.` (3D raster), `db.`
(database), `m.` (miscellaneous), `ps.` (PostScript).

## Commit Messages

See `doc/development/github_guide.md` for the full Git workflow. Commit
message rules:

- Start with a prefix matching the category regexps in `utils/release.yml`
  (e.g., `r.slope.aspect:`, `grass.script:`, `CI:`)
- After the prefix, start with a capital letter — unless the first word is an
  identifier that is conventionally lowercase (e.g., `r.info` or `gs`)
- Use plain ASCII only, no double spaces after periods
- Write in imperative mood (e.g., "Add support for X", not "Added" or "Adds")
- Do not add AI co-authors; the human author is responsible for the code.
  However, larger use of AI should be acknowledged in the commit message
  and/or PR description, similarly to how a book or a discussion with a
  human collaborator would be acknowledged.

## AI Use Policy

See `CONTRIBUTING.md` for the full AI use policy. Key points: AI-assisted
development is acceptable, but contributors must test all code, understand
their submissions, and disclose AI assistance when substantial algorithms
or logic were AI-generated.
