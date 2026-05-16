# Copilot Instructions (GRASS GIS)

GRASS GIS is a geospatial processing engine with 500+ tools (modules) for
raster, vector, imagery, temporal, and 3D analysis. Each tool is self-contained
with source, docs, Makefile, and tests in one directory.

## Repository Layout

| Path | Contents |
| ---- | -------- |
| `raster/r.*/` | Raster tools (C, e.g. `raster/r.slope.aspect/`) |
| `vector/v.*/` | Vector tools (C/Python) |
| `temporal/t.*/` | Temporal framework tools (Python) |
| `display/d.*/` | Display/rendering tools (C) |
| `general/g.*/` | General management tools |
| `db/db.*/` | Database tools |
| `imagery/i.*/` | Imagery processing tools |
| `scripts/` | Python script tools (e.g. `scripts/r.grow/r.grow.py`) |
| `lib/` | Shared C libraries (`lib/gis/`, `lib/raster/`, `lib/vector/`, etc.) |
| `python/grass/` | Python packages (`script/`, `pygrass/`, `gunittest/`, `temporal/`, `tools/`) |
| `gui/wxpython/` | wxPython GUI |

## Module Directory Structure

Each module directory contains:

- `Makefile` — links to `include/Make/Module.make` (C)
  or `include/Make/Script.make` (Python)
- `main.c` or `<tool>.py` — source code
- `<tool>.md` — documentation (Markdown, **no header/footer**)
- `tests/` — pytest tests (preferred for new tests)
- `testsuite/` — legacy gunittest tests

## Build System

GRASS has two build systems. **CMake + Ninja** is recommended for development:

```bash
cmake -S . -B build -G Ninja -DCMAKE_INSTALL_PREFIX="$HOME/install"
cmake --build build -j$(nproc)
cmake --install build
```

Autotools is also supported:
`./configure --prefix=$PREFIX && make -j$(nproc) && make install`

## Testing

Two test frameworks coexist:

**pytest** (preferred for new tests) — files in `<module>/tests/`:

```python
# tests/conftest.py — typical fixture pattern
import grass.script as gs
from grass.tools import Tools

@pytest.fixture
def xy_dataset_session(tmp_path):
    project = tmp_path / "xy_test"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session, \
         Tools(session=session) as tools:
        tools.g_region(s=0, n=5, w=0, e=6, res=1)
        tools.r_mapcalc(expression="rows_raster = row()")
        yield session

# tests/my_tool_test.py
def test_output(xy_dataset_session):
    tools = Tools(session=xy_dataset_session, consistent_return_value=True)
    tools.r_slope_aspect(elevation="rows_raster", slope="slope_out")
    stats = tools.r_univar(map="slope_out", format="json")
    assert stats["min"] == 45
```

**gunittest** (legacy) — files in `<module>/testsuite/`, based on
`grass.gunittest.case.TestCase` with assertions like `assertModule()`,
`assertRasterMinMax()`.

Run tests:

```bash
# pytest (from build directory or with GRASS in PATH)
pytest raster/r.slope.aspect/tests/
# gunittest (requires NC sample dataset)
grass --tmp-project XY --exec python3 -m grass.gunittest.main \
    --grassdata $HOME --location nc_spm_full_v2beta1 --min-success 100
```

## Critical Coding Conventions

These are GRASS-specific rules that differ from general practice. See
`.github/instructions/` for full details.

### Python Tools

- **Import**: always `import grass.script as gs`
- **Translatable messages**: use `_()` with `str.format()`, never f-strings:
  `gs.warning(_("Raster map <{}> not found.").format(name))`
- **Non-translatable strings**: f-strings are fine:
  `expression = f"{output} = {input} * 3"`
- **Parser interface**: defined via special comments, not argparse:

  ```python
  # %Module
  # % description: Short tool description
  # % keyword: raster
  # %end
  # %option G_OPT_R_INPUT
  # %end
  # %option G_OPT_R_OUTPUT
  # %end
  ```

- **Computational region**: never change globally. Use context manager:
  `with gs.RegionManager(raster=input_map): ...`
- **Raster mask**: never set/remove globally. Use:
  `with gs.MaskManager(): gs.run_command("r.mask", raster=mask)`
- **Temporary maps**: use `gs.append_node_pid("tmp_name")` + `atexit` cleanup
- **Record history**: call `gs.raster_history(output)` or `gs.vector_history(output)`
- **Overwrite**: never overwrite outputs unless the user passes `--overwrite`
- **Mapsets**: outputs go to the current mapset; inputs may come from any mapset.
  Do not hard-code file paths for geodata; use GRASS map names.
- **Messages**: map names in `<angle brackets>`, file paths in `'single quotes'`,
  capitalize, no contractions, use `gs.message()` / `gs.fatal()` / `gs.warning()`
  (never `print()` for messages)

### C Tools

- Use GRASS APIs: `G_malloc()`, `G_fatal_error()`, `G_warning()`, `Rast_open_old()`
- Header include order: system → non-core libs → `grass/*.h` → local headers
- Format with `clang-format` (v18+); snake_case for function names
- Exit with `EXIT_SUCCESS` / `EXIT_FAILURE`

### Documentation (`<tool>.md`)

Required sections: DESCRIPTION, SEE ALSO, AUTHORS.
Suggested: NOTES, EXAMPLES. Tool names in italics (`*r.slope.aspect*`),
flags/params bold (`**-n**`, `**input**`), shell values in backticks.
See `.github/instructions/docs.instructions.md` for full markup guide.

## Formatting & Linting

Pre-commit runs all checks. Install once, then it runs on every commit:

```bash
python -m pip install pre-commit && pre-commit install
# Manual run:
pre-commit run --all-files
# Target specific files:
pre-commit run --files raster/r.slope.aspect/*
```

Key tools: `ruff format` + `ruff check` (Python), `clang-format` (C/C++),
`markdownlint` (docs). Config in `pyproject.toml`, `.clang-format`,
`.pre-commit-config.yaml`.

## Key Reference Files

- Python conventions: `.github/instructions/python.instructions.md`
- C conventions: `.github/instructions/c.instructions.md`
- Doc markup guide: `.github/instructions/docs.instructions.md`
- General rules: `.github/instructions/general.instructions.md`
- [GRASS Python API docs](https://grass.osgeo.org/grass-devel/manuals/libpython/)
- [GRASS C Programmer's Manual](https://grass.osgeo.org/programming8/)
