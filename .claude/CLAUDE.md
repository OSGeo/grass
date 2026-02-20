# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working
with code in this repository.

## Overview

GRASS is a large, multi-language geospatial processing engine. The codebase
is primarily C (core libraries and tools) and Python (high-level API, scripts,
GUI, tests). Tools follow a `<type>.<name>` naming convention: `r.*` (raster),
`v.*` (vector), `g.*` (general), `d.*` (display), `i.*` (imagery), `t.*`
(temporal), `r3.*` (3D raster).

## Build

On the `main` branch, Autotools and CMake are equivalent. Older release
branches support Autotools only, and full documentation builds (MkDocs) also
require Autotools.

**Autotools:**

```bash
CFLAGS="-g -Wall" ./configure    # configure with debug flags
make                              # compile everything
make install                      # install
make libs                         # compile libraries only (faster for tool development)
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

**Run all pytest tests:**

```bash
pytest
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

pytest configuration is in `pyproject.toml`. Tests require a running GRASS
session with a project/mapset; see existing `conftest.py` files for session
fixture patterns using `gs.create_project()` and `gs.setup.init()`.

## Linting and Formatting

**Python — format and check:**

```bash
ruff format                       # format Python files
ruff check .                      # lint
ruff check . --fix                # lint and auto-fix
```

**C/C++ — format:**

```bash
clang-format -i <file.c>         # format a C file
```

**Pre-commit (recommended — runs both Python and C formatting automatically):**

```bash
pre-commit install                # install hooks once per repo
pre-commit run --all-files        # run all hooks manually
pre-commit run --files raster/r.sometool/*  # run on specific files
```

**Additional checks:**

```bash
flake8 python_file.py             # PEP8 compliance check
pylint <file.py>                  # additional static analysis (not a primary compliance tool)
codespell .                       # spell check
```

Non-compliance with ruff/flake8 rules is mainly a legacy GUI code issue. New
code is expected to be clean. Pylint is an extra linter for finding specific
issues beyond what ruff catches; full Pylint compliance is not enforced.

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
  `tools.r_slope_aspect(...)`). Do not use in `grass.script` or anything
  `grass.script` depends on (circular dependency). Not to be confused with
  the object-oriented API of `grass.pygrass`.
- `grass.pygrass` — object-oriented API; provides `Raster`, `Vector` classes
  and a lower-level module interface
- `grass.temporal` — space-time dataset (STRDS/STVDS) management. Import as
  `import grass.temporal as tgis`.

### Tool Structure

Each tool (e.g., `raster/r.slope.aspect/`) contains:

- `main.c` (or `<name>.py` for Python scripts) — entry point
- `Makefile` — follows a standard template including
  `$(MODULE_TOPDIR)/include/Make/Module.make`
- `<name>.md` — documentation source (no header/footer; Markdown rendered by
  MkDocs)
- `<name>.html` — documentation in HTML format; must be committed alongside
  the `.md` file to provide basic HTML access
- `tests/` — pytest tests
- `testsuite/` — gunittest tests (legacy)

### Python Scripts (`scripts/`)

Python-based tools (e.g., `scripts/r.mask/`) use `grass.script` and
`g.parser` for option parsing. They follow the same tool structure as C tools.

### GUI (`gui/wxpython/`)

wxPython desktop application. Large, mostly independent from core libraries.
Follows wxPython style guide rather than standard GRASS Python conventions in
some areas.

## Key Coding Conventions

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

### Tests

- Module-level globals in test files do not need a leading underscore; test
  modules are not public APIs.

### Python

- Import `grass.script` as `gs` (enforced by ruff `ICN001` rule)
- Import `grass.temporal` as `tgis`
- Use `str.format()` for translatable user messages (not f-strings):
  `gs.warning(_("Map <{}> not found.").format(name))`
- Use f-strings for non-translatable strings
- When using `grass.script`: use `gs.run_command()`, `gs.parse_command()`,
  `gs.read_command()` rather than `subprocess` directly
- When using `grass.tools`: use the `Tools` class with a session context
  manager
- Use `gs.fatal()` to exit with error, `gs.warning()` for warnings,
  `gs.message()` for info; never `print()` for informational output
- Wrap region changes in `gs.RegionManager()` context manager to avoid side
  effects
- Wrap mask changes in `gs.MaskManager()` context manager
- Use `gs.append_node_pid("tmp_name")` for temporary map names; register
  cleanup with `atexit`
- Never change the global computational region from a tool without using a
  context manager
- Data is organized in **projects** (formerly called "locations") and mapsets

### C

- Use `G_malloc()`, `G_calloc()`, `G_free()` instead of standard C equivalents
- Use `G_fatal_error()`, `G_warning()`, `G_message()` for output
- Use `EXIT_SUCCESS`/`EXIT_FAILURE` return values from `main()`
- Include order: system headers → non-core system libs → GRASS headers
  (`grass/gis.h`, etc.) → local headers; groups alphabetically sorted,
  separated by blank lines
- Snake case for function names (GNU naming convention)
- All new/modified C files formatted with `clang-format`

### Documentation (tool doc files)

Each tool must have both `<tool>.md` and `<tool>.html` committed to the
repository. The `.md` is the source of truth; the `.html` provides basic HTML
access and must be kept in sync.

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
parameters from scratch. Prefer a `format` option over multiple exclusive
flags for output format selection. Output must always go to the current
mapset. Tools must not overwrite existing maps unless the user provides
`--overwrite`.

Tool name prefixes: `r.` (raster), `v.` (vector), `g.` (general), `d.`
(display), `i.` (imagery), `t.` (temporal), `r3.` (3D raster), `db.`
(database), `m.` (miscellaneous), `ps.` (PostScript).

## Commit Messages

Commit messages must start with a prefix that matches the category regexps
defined in `utils/release.yml` (e.g., `r.slope.aspect:`, `grass.script:`,
`CI:`). After the prefix, the message should start with a capital letter —
unless the first word is an identifier that is conventionally lowercase (e.g.,
a tool name like `r.info` or a Python name like `gs`). Use plain ASCII only
and no double spaces after periods. Write the message in imperative mood
(e.g., "Add support for X", not "Added" or "Adds"). Do not add AI co-authors;
the human author is responsible for the code.

## Development Documentation

Developer guides are in `doc/` and `doc/development/`:

- `doc/development/style_guide.md` — coding style for Python, C/C++, and
  tool documentation
- `doc/development/github_guide.md` — Git workflow and pull request process
- `doc/development/branching_how-to.md` — branch management
