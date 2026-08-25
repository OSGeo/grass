# Testing

New code should come with tests, and a bug fix should come with a test which
fails without the fix; when the code being fixed has no tests at all, add a
test for its basic functionality as well, so it is clear the fix did not break
it.

GRASS has two testing frameworks. Use _pytest_ for everything new. Use the
older _grass.gunittest_ framework when the test needs the North Carolina
sample dataset, or when it needs data supplied as files. Neither is currently
possible with _pytest_.

The two live side by side in the source tree: _pytest_ tests go into a
`tests` directory next to the code they test, _grass.gunittest_ tests into a
`testsuite` directory.

## Running tests

Running tests requires a built and installed GRASS with the `grass` command
available. Point Python at that installation first:

```bash
export PYTHONPATH="$(grass --config python_path):${PYTHONPATH}"
export LD_LIBRARY_PATH="$(grass --config path)/lib:${LD_LIBRARY_PATH}"
```

Then run the tests of the tool you are working on:

```bash
pytest raster/r.slope.aspect/tests/
```

Running `pytest` without a path from the top of the source tree collects the
`testsuite` directories as well, because the collection patterns in
`pyproject.toml` include them. Most _grass.gunittest_ tests need a GRASS
session and the sample dataset, so they fail when run this way. Exclude them
as the CI does:

```bash
pytest -k "not testsuite"
```

A _grass.gunittest_ test runs inside a GRASS session instead. Give it a
temporary mapset, which is created and removed for each run:

```bash
cd raster/r.slope.aspect/testsuite
grass --tmp-mapset ~/grassdata/nc_spm_08_grass7/ \
    --exec python test_r_slope_aspect.py
```

The test files are not executable, so `--exec` needs `python` in front of
the file name, and some of them load data files by relative path, so run them
from their own directory.

## Writing pytest tests

### File placement and naming

Tests of a tool go next to it, in a `tests` directory, in a file named after
the tool with dots replaced by underscores, e.g.
`raster/r.slope.aspect/tests/r_slope_aspect_test.py`. When a tool needs more
than one file, add the feature or domain under test, e.g.
`r_slope_aspect_memory_test.py`. Tests of libraries and packages use the full
name of the unit the same way, e.g. `lib/gis/tests/lib_gis_env_test.py`.

Both `<name>_test.py` and `test_<name>.py` are collected, so either is fine;
`<name>_test.py` is the more common form in the source tree.

### Session fixture

Most tests need a GRASS session with a project and some data, so they set one
up in a fixture. Putting the fixture in a `conftest.py` file next to the tests
is not required, but it is common, because it keeps the setup separate from
the tests and lets several test files share it. This is the session fixture
used by the _r.slope.aspect_ tests, in
`raster/r.slope.aspect/tests/conftest.py`:

```python
import os

import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.fixture
def xy_dataset_session(tmp_path):
    """Active session in an XY project with a raster (scope: function)"""
    project = tmp_path / "xy_test"
    gs.create_project(project)
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        tools.g_region(s=0, n=5, w=0, e=6, res=1)
        tools.r_mapcalc(expression="rows_raster = row()")
        yield session
```

Always pass `env=os.environ.copy()` to `gs.setup.init()`, never call
`gs.setup.init(project)` on its own. Without it the session modifies the
global environment, and since all tests run in the same process, a later test
which copies `os.environ` inherits whatever earlier tests left behind.

Create an XY project, as above, unless the test is about coordinate
reference systems or needs real ground units; then pass `epsg` to
`gs.create_project()`.

Creating a project and its data for every test is fine when the data is small,
as above. When the data is large, plentiful, or expensive to compute, give the
session fixture module scope so the tests share it, and add a function-scoped
fixture using `TemporaryMapsetSession` from `grass.experimental`. Each test
then gets its own mapset in the shared project, which keeps the tests isolated
from each other. This suits most tool tests; tests of data management tools
often need something else. `raster/conftest.py` follows this pattern.

Fixtures in a `conftest.py` are available to all tests below it in the
directory tree, so a fixture useful to a whole group of tools can live higher
up. The `raster`, `vector`, and `scripts` directories each have one, and many
`tests` directories use those instead of defining their own. The source tree
has many more examples than this page can show.

### Test functions

A test asks the fixture for the session and runs tools against it:

```python
from grass.tools import Tools


def test_slope_of_constant_slope_raster(xy_dataset_session):
    """Slope of a raster with a constant slope is constant"""
    tools = Tools(session=xy_dataset_session)
    tools.r_slope_aspect(elevation="rows_raster", slope="slope")
    stats = tools.r_univar(map="slope", format="json")
    assert stats["min"] == 45
    assert stats["max"] == 45
```

A result gives access to the output in several ways: `text`, `stdout`,
`keyval` for `format="shell"`, and `json` for `format="json"`, which can also
be subscripted directly as above. See the `Tools` documentation for the rest.

### Test data

Where possible, generate small, deterministic data instead of using data
files. Raster algebra with _r.mapcalc_ covers most needs, using `row()` and
`col()` for predictable values, e.g. `elevation = 6 - col()`. Vector data can
come from _v.random_ with a fixed `seed`, or from _v.in.ascii_ and
_r.in.ascii_, which accept an `io.StringIO` object as their input parameter.
NumPy arrays can be passed to tools and requested back from them through
_grass.tools_.

When the test needs specific data which cannot be generated, the data has to
be supplied as files, which is currently possible only with _grass.gunittest_
in a `testsuite` directory.

### Tests which run in parallel

Tests run in parallel, several per process, so a test which changes
`os.environ` can break unrelated tests running next to it. This applies to
rendering and to _grass.temporal_, which reads the session from the global
environment. Mark such a test so it is run on its own:

```python
@pytest.mark.needs_solo_run
def test_something_which_changes_the_environment(xy_dataset_session):
    ...
```

## More information

- [grass.gunittest documentation](https://grass.osgeo.org/grass-devel/manuals/libpython/gunittest_testing.html)
  for writing and running _grass.gunittest_ tests
- [testsuite/README.md](https://github.com/OSGeo/grass/blob/main/testsuite/README.md)
  for the sample datasets and continuous integration
- [pytest documentation](https://docs.pytest.org/) for fixtures,
  `parametrize`, and `pytest.raises`
- [AGENTS.md](https://github.com/OSGeo/grass/blob/main/AGENTS.md) for
  additional instructions for AI assistants and agents
