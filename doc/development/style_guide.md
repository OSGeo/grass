# GRASS Programming Style Guide

1. [Code Style and Formatting](#code-style-and-formatting)
    1. [Python](#python)
    2. [C and C++](#c-and-c)
    3. [Using pre-commit](#using-pre-commit)
    4. [Documentation](#documentation)
2. [Best Practices](#best-practices)
    1. [General](#general)
    2. [Python scripts](#developing-python-scripts)
    3. [GRASS Addons](#developing-grass-addons)
    4. [GRASS GUI](#developing-grass-gui)

## Code Style and Formatting

### Python

We follow the [PEP8](https://www.python.org/dev/peps/pep-0008/) style guide for
Python. Docstrings follow [PEP257](https://www.python.org/dev/peps/pep-0257/) and
use
[Sphinx style](https://sphinx-rtd-tutorial.readthedocs.io/en/latest/docstrings.html).

We use the following tools in the continuous integration to ensure compliance
with PEP8, consistent formatting, and readable and error-free code:

- [Flake8](https://flake8.pycqa.org/en/latest/) tool to ensure compliance with
  PEP8
- [Ruff](https://docs.astral.sh/ruff/) linter and code formatter to ensure
  code quality and consistent code formatting
- [Pylint](https://pylint.readthedocs.io/en/latest/) static code analyser to
  find bad code practices or errors

Note that while the entire GRASS code base is formatted with `ruff format`,
full compliance with PEP8, Flake8, and Pylint practices is still work in progress.

See [using pre-commit](#using-pre-commit) for pre-commit setup and usage to
simplify performing of these checks.

#### Formatting

Use Ruff's formatter to format files:

```bash
ruff format
```

#### Flake8

Use Flake8 to check formatting and basic issues in all files:

```bash
flake8 python_file.py
```

The root directory contains [.flake8](../../.flake8) configuration file which
contains a less strict configuration for legacy code. It will be used by default
when running Flake8 within GRASS source code. For new files, you can use the
default configuration:

```bash
flake8 --isolated --max-line-length=88 {path_to_python_file}
```

For specific, temporary issues, you can explicitly specify which errors or
warnings to ignore:

```bash
flake8 --ignore=E203,E266,E501 --max-line-length=88 python_file.py
```

### C and C++

C and C++ code is formatted with
[ClangFormat](https://clang.llvm.org/docs/ClangFormat.html). Contributions are
expected to be formatted with `clang-format` (currently with version 18+). The
most convenient method to install clang-format and format files is
[using pre-commit](#using-pre-commit).

Alternatively, using separately installed clang-format on modified files:

```bash
clang-format -i <new_or_modified_file.c>
```

The ClangFormat settings for the repo are defined in
[.clang-format](../../.clang-format).

If using pre-commit is not an option, for whatever reason, there is a helper
script [grass_clang_format.sh](./utils/grass_clang_format.sh), which simplifies
bulk reformatting.

#### Order of include headers

In general, headers should be included in the order:

1. Core system headers (stdio.h, ctype.h, ...)
2. Headers for non-core system components (X11, libraries).
3. GRASS headers (grass/gis.h, grass/glocale.h, ...)
4. Headers for the specific library/program (geodesic.h, ...)

Each class of headers has an obligation to be compatible with those above it in
the list, but not those below it. The header groups should be alphabetically
sorted and separated by a newline.

```c
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/raster.h>

#include "local_proto.h"
#include "mask.h"
```

#### Naming conventions

Use function names which fulfill the official [GNU naming
convention](https://www.gnu.org/prep/standards/html_node/Names.html). Instead of
naming a function like: `MyNewFunction()` use snake case: `my_new_function()`.

### Using pre-commit

It is highly recommended to install and use [pre-commit](https://pre-commit.com)
before submitting any new or modified code or any other content. The pre-commit
uses Git hooks to check validity and executes automated formatting for
a range of file formats, including C/C++ and Python. Pre-commit installs
all necessary tools in a virtual environment upon first use.

If you never used pre-commit before, you must start by installing it on your
system. You only do it once:

```bash
python -m pip install pre-commit
```

Pre-commit must then be activated in the code repository. Change the directory
to the root folder and use the `install` command:

```bash
cd <grass_source_dir>

# once per repo
pre-commit install
```

Pre-commit will then be automatically triggered by the `git commit` command. If
it finds any problem it will abort the commit and try to solve it automatically.
In that case review the changes and run again `git add` and
`git commit`.

It is also possible to run pre-commit manually, e.g:

```bash
pre-commit run --all-files
pre-commit run clang-format --all-files
pre-commit run ruff-format --all-files
```

Or to target a specific set of files:

```bash
pre-commit run --files raster/r.sometool/*
```

The pre-commit hooks set is defined in
[.pre-commit-config.yaml](../../.pre-commit-config.yaml).

It is possible to temporally disable the pre-commit hooks in the repo, e.g. while
working on older branches:

```bash
# backporting...
pre-commit uninstall
```

And to reactivate pre-commit again:

```bash
git switch main
pre-commit install
```

### Documentation

There are three types of documentation: C API, Python API and tool documentation.

#### C API documentation

We
[use doxygen and document the functions](https://grass.osgeo.org/programming8/)
directly in the source code. See `lib/gis/open.c` and `lib/gis/gislib.dox` for
examples.

#### Python API documentation

Python API documentation is written in reStructuredText (reST) which is
compiled with Sphinx (see
[grass package documentation](https://grass.osgeo.org/grass-devel/manuals/libpython/))

```python
def func(arg1, arg2):
    """Summary line.

    Extended description of function.

    :param int arg1: Description of arg1.
    :param str arg2: Description of arg2.
    :raise: ValueError if arg1 is equal to arg2
    :return: Description of return value
    :rtype: bool

    Example:

    >>> a=1
    >>> b=2
    >>> func(a,b)
    True
    """

    if arg1 == arg2:
        raise ValueError('arg1 must not be equal to arg2')

    return True
```

See
[Sphinx docstring formatting](https://sphinx-rtd-tutorial.readthedocs.io/en/latest/docstrings.html)
for more details.

#### Tool documentation

Documentation of a tool should come with clear descriptions, hints on the
implemented algorithm and example(s) with figures.

Each tool (also called "module") comes with its own manual page written in
Markdown. The Markdown file contains **no header nor footer**. The complete
file is autogenerated during the compilation process (with ``--md-description
parameter``). In order to make sure that manuals build without issues, we run
a Markdown linter on all pull requests.

Name the documentation file `'<tool>.md'`, e.g., if the tool is named
r.example, the documentation file should be named `r.example.md`.

##### Markup style guide

The structure consists of several required and optional sections:

```markdown
## DESCRIPTION
<!-- required -->

## NOTES
<!-- suggested -->

## EXAMPLES
<!-- suggested -->

## TODO
<!-- optional -->

## KNOWN ISSUES
<!-- optional -->

## REFERENCES
<!-- optional -->

## SEE ALSO
<!-- required -->

## AUTHORS
<!-- required -->
```

Sections _Notes_, _Examples_, _References_, and _Authors_ can be also in
singular form (e.g, _Note_).

Note that Markdown is converted to html using [MkDocs](https://www.mkdocs.org/).
See also [supported Markdown elements by MkDocs](https://www.markdownguide.org/tools/mkdocs/)
and CI-enforced [linter rules](https://github.com/markdownlint/markdownlint/blob/main/docs/RULES.md).

More notes on markup:

- Tool names (i.e., v.category) should be emphasized like `*v.category*`.
- Flags and parameter names written in boldface like `**-f**` and
  `**input**`.
- Shell commands, names, values, etc. should use `` `42` ``.
- Emphasized phrases `*should use italics*`.
- In order to minimize potential git merge conflicts, please break a line at
  approximately 70-80 chars.
- A line break in the resulting HTML is created by adding two spaces at the end
  of a line. Use this sparingly and only when necessary.

Examples should be coded like this:

````markdown
```sh
v.to.db map=soils type=area option=area column=area_size unit=h
```

```python
gs.run_command("g.region", flags="p")
```
````

The `SEE ALSO` section of each page should be alphabetized:

```markdown
*[d.shade](d.shade.md), [r.shade](r.shade.md)*
```

Alternatively, the section can provide details on how each of the linked tools
or pages is relevant:

```markdown
*[r.shade](r.shade.md) for computing shaded relief,  
[d.shade](d.shade.md) for displaying shaded relief with other data,  
[g.region](g.region.md) for managing the resolution.*
```

In this case, the list can be ordered thematically rather than alphabetically.
Either all tools should have the description or none (do not mix the styles).

##### Images

**Naming convention:** `tool_name.png` or `tool_name_keyword.png` (in both cases,
dots in tool name are replaced by underscores)

Examples:

- `d_geodesic.png`
- `r_resamp_stats_6m_20m.png`
- `g_gui_rlisetup_8.png`
- `v_clean_rmsa.png`

**Image size:** ideally **600 pixel width** (height depends on that), use e.g.
ImageMagic:

```bash
mogrify -resize 600x file.png
```

Smaller images are also possible when appropriate, e.g. when a lot of images are
included or they are something special, e.g. equations, icons or simple
diagrams. Larger images are supported, too, see below for an optimal inclusion
into the Markdown page.

Please **compress** PNG images with:

```bash
# color quantization
# optional, but usually worth it
# note: may change colors
pngnq -n 128 -s 3 file.png

# shuffle original and quantitized image names
mv file.png file_ORIG.png
mv file-nq8.png file.png

# compress better (lossless)
optipng -o5 file.png
```

**Format:** Images should be ideally in PNG (well, JPG and GIF is allowed as well
when appropriate but usually it is not!). Vector graphics should be included in
pages as raster images (i.e. PNGs) for portability but the original format
(preferably SVG) should be committed to the repository as well.

Adding the image to the HTML page (r.viewshed example, the screenshot is shown
with a width of 600 pixel but it is clickable in the manual page). If a larger
image is displayed as shrunk, both **width** and **height** HTML parameters
(values must be calculated according to the picture size!) should be set:

```markdown
![r.viewshed example](r_viewshed.png)  
*Figure: Viewshed shown on shaded terrain (observer position in the
north-east quadrant with white dot; 5m above ground)*
```

_Note the 2 spaces after the image._

## Best Practices

### General

#### Computational Region

Tools typically **do not change the computational region based on the input
data**. Raster processing tools should respect the current computational region.

**Why?** Users should be able to re-run a command or workflow with different
computational regions to, e.g., test processing in a small area and then move to
a larger one. Also, changing the current region globally can break parallel
processing.

**Exceptions**: Tools importing data typically import the entire dataset, respecting
of the region may be implemented as an optional feature (e.g., r.in.gdal). This
is to avoid, e.g., importing data under finer resolution than their native
resolution. Another exception is raster processing where alignment
of the cells plays a crucial role and there is a clear answer to how the
alignment should be done. In that case, the tool may change the resolution. Some
tools, such as r.mapcalc, opt for providing additional computational region
handling policies. Finally, some operations are meant to use all the data, e.g.,
creating metadata, these operations should not use the current computational
region.

If you need to change the computational region, there are ways to change it only
within your script, not affecting the current region.

#### Mapsets

**Output data should be always written to the current mapset**. This is ensured
by build-in GRASS mechanisms, so there is nothing which needs to be done in the
tool. If a tool modifies inputs, the input must be in the current mapset.

The tool should accept inputs from any mapset in the current project. The
user-provided name **may or may not include mapset name** and the tool needs to
respect that.

#### Input and Output Geospatial Data Format

An analytical tool should read and write geospatial data as GRASS raster or
vector maps. Importing from and exporting to other data formats should be left
to dedicated import and export tools, e.g., _v.import_. The exceptions are
import and export of data, e.g., _r.in.xyz_.

Processing and analytical tools can then use simple names to refer to the
data within GRASS projects instead of file paths. This follows the separation of
concerns principle: format conversion and CRS transformations are separate from
analysis.

#### Overwriting Existing Data

A tool should not overwrite existing data unless specified by the user using the
`--overwrite` flag. The GRASS command line parser automatically checks for
output data (raster, vector maps) existence and ends the tool execution with a
proper error message in case the output already exists. If the flag is set by
the user (`--overwrite` in command line, `overwrite=True` in Python), the parser
enables the overwriting for the tool.

The `--overwrite` flag can be globally enabled by setting the environment variable
`GRASS_OVERWRITE` to 1. Notably, the GRASS session from _grass.jupyter_ sets
`GRASS_OVERWRITE` to 1 to enable re-running of the cells and notebooks.

#### Mask

GRASS GIS has a global mask managed by the _r.mask_ tool and represented by a
raster called MASK by default. Raster tools called as a subprocess will automatically
respect the globally set mask when reading the data. For outputs, respecting of
the mask is optional.

Tools should generally respect the global mask set by a user. If the mask set
by the user is not respected by a tool, the exact behavior should be described
in the documentation. On the other hand, ignoring mask is usually the desired
behavior for import tools which corresponds with the mask being applied only
when reading existing raster data in a project.

Tools **should not set or remove the global mask** to prevent unintended
behavior during interactive sessions and to maintain parallel processing
integrity. If a tool requires a mask for its operation, it should implement
a temporary mask using _MaskManager_ in Python or by setting the `GRASS_MASK`
environment variable.

Generally, any mask behavior should be documented unless it is the standard case
where masked cells do not participate in the computation and are represented as
NULL cells (no data) in the output.

#### Formatting messages

Put raster, vector maps, imagery groups etc. in brackets:

```text
Raster map <elevation> not found.
```

Put file paths, SQL queries into single quotes:

```text
File '/path/to/file.txt' not found.
```

First letter should be capitalized.

Avoid contractions (cannot instead of can't).

Be consistent with periods. Complete sentences or all parts of a message with
multiple sentences should end with periods. Short phrases should not.
Punctuated events, such as errors, deserve a period, e.g., _"Operation
complete."_ Phrases which imply ongoing action should have an ellipse, e.g.,
 _"Reading raster map..."_.

### Developing Python scripts

#### Import Python Scripting Library

```python
import grass.script as gs

gs.run_command(...)
```

#### String formatting

User messages should be translatable and for formatting, use `str.format()`, not
f-strings:

```python
gs.warning(_("Raster map <{}> not found.").format(input_map))
```

For strings that are not translatable, use f-strings:

```python
r_mapcalc_expression = f"{output_map} = {input_map} * 3"
```

#### Temporary files

To create a temporary file, use `NamedTemporaryFile` with a context manager. In
this example, we open a temporary file for writing, write something and then we
can use it in another tool. Once we do not need it anymore, we need to delete it
ourselves.

```python
import tempfile

with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
    file_path = tmp_file.name
    tmp_file.write(...)

gs.try_remove(file_path)
```

#### Changing computational region

If a tool needs to change the computational region for part of the computation,
temporary region in Python API is the simplest way to do it:

```python
gs.use_temp_region()  # From now on, use a separate region in the script.
# Set the computational region with g.region as needed.
grass.run_command('g.region', raster='input')
gs.del_temp_region()
# Original region applies now.
```

This makes any changes done in the tool local for the tool without influencing
other tools running in the same session.

If you need even more control, use the GRASS_REGION environment variable which
is passed to subprocesses. Python API has functions which help with the setup:

```python
os.environ["GRASS_REGION"] = gs.region_env(raster=input_raster)
```

If different subprocesses need different regions, use different environments:

```python
env = os.environ.copy()
env["GRASS_REGION"] = gs.region_env(raster=input_raster)
gs.run_command("r.slope.aspect", elevation=input_raster, slope=slope, env=env)
```

This approach makes the computational region completely safe for parallel
processes as no region-related files are modified.

#### Changing raster mask

The _MaskManager_ in Python API provides a way for tools to change, or possibly
to ignore, a raster mask for part of the computation.

In the following example, _MaskManager_ modifies the global system environment
for the tool (aka _os.environ_) so that custom mask can be applied:

```python
# Previously user-set mask applies here (if any).
gs.run_command("r.slope.aspect", elevation=input_raster, aspect=aspect)

with gs.MaskManager():
    # Only the mask we set here will apply.
    gs.run_command("r.mask", raster=mask_raster)
    gs.run_command("r.slope.aspect", elevation=input_raster, slope=slope)
# Mask is disabled and the mask raster is removed at the end of the with block.

# Previously user-set mask applies here again.
```

Because tools should generally respect the provided mask, the mask in a tool
should act as an additional mask. This can be achieved when preparing the new
mask raster using a tool which reads an existing raster:

```python
# Here we create an initial mask by creating a raster from vector,
# but that does not use mask.
gs.run_command(
    "v.to.rast", input=input_vector, where="name == 'Town'", output=town_boundary
)
# So, we use a raster algebra expression. Mask will be applied if set
# because in the expression, we are reading an existing raster.
gs.mapcalc(f"{raster_mask} = {town_boundary}")

with gs.MaskManager():
    gs.run_command("r.mask", raster=mask_raster)
    # Both user mask and the town_boundary are used here.
    gs.run_command("r.slope.aspect", elevation=input_raster, slope=slope)
```

To disable the mask, which may be needed, e.g., in processing steps of
an import tool, we can do:

```python
# Mask applies here if set.
gs.run_command("r.slope.aspect", elevation=input_raster, aspect=aspect)

with gs.MaskManager():
    # No mask was set in this context, so the tool runs without a mask.
    gs.run_command("r.slope.aspect", elevation=input_raster, slope=slope)

# Mask applies again.
```

If needed, tools can implement optional support for a user-set raster mask by
passing or not passing the current name of a mask obtained from _r.mask.status_
and by preparing the internal mask raster beforehand with the user mask active.

If different subprocesses, running in parallel, use different masks,
it is best to create mask rasters beforehand (to avoid limitations of _r.mask_
and the underlying _r.reclass_ tool). The name of the mask raster can then be
passed to the manager:

```python
env = os.environ.copy()
with gs.MaskManager(mask_name=mask_raster, env=env):
    gs.run_command("r.slope.aspect", elevation=input_raster, slope=slope, env=env)
```

#### Temporary Maps

Using temporary maps is preferred over using temporary mapsets. This follows the
rule that writing should be done only to the current mapset. Some users may have
write permissions only for their mapsets, but not for creating other mapsets.

The following script creates a temporary name using `gs.append_node_pid` which
uses node (computer) name and process identifier to create unique, but
identifiable name. The temporary maps are removed when the script ends.

```python
import atexit

import grass.script as gs


def remove(name):
    gs.run_command(
        "g.remove",
        type="raster",
        name=name,
        flags="f",
        quiet=True,
        errors="ignore",
    )



def main():
    temporary = gs.append_node_pid("tmp_mapcalc")
    atexit.register(remove, temporary)

    gs.mapcalc(f"{temporary} = rand(1, 10)")


if __name__ == "__main__":
    main()
```

#### Checking inputs of a tool

Use gs.findfile() when there is a need to test if a map exists.

```python
# test for input raster map
result = gs.find_file(map_name, element='raster')
if not result['file']:
    gs.fatal(_("Raster map <{}> not found").format(map_name))

# test for input vector map
result = gs.find_file(map_name, element='vector')
if not result['file']:
    gs.fatal(_("Vector map <{}> not found").format(map_name))
```

Tools need to accommodate input map names with (_elevation_) and without mapset
(_elevation@PERMANENT_). If you need only the map name without mapset, you can
do:

```python
map_name = map_name.split("@")[0]
```

If you need the full name or the mapset only, use _gs.findfile_:

```python
file_info = gs.find_file(map_name, element="raster")
full_name = file_info["fullname"]
name = file_info["name"]
mapset = file_info["mapset"]
```

#### Messages

For any informational output, use the _gs.message_ function or _gs.verbose_. For
error messages, use _gs.fatal_ (ends execution) or _gs.error_ (just prints error,
so additional code needs to handle next steps and communicate them to the user).
For warnings, use  _gs.warning_. For debugging purposes use _gs.debug_.

```py
# normal message:
gs.message(_("Done."))

# verbose message:
gs.verbose(_("Computation finished successfully."))

# warning:
gs.warning(_("No input values found, using default values."))

# error:
gs.error(_("No map found."))

# fatal error:
# prints error and exits or raises exception (use set_raise_on_error to set the behavior)
gs.fatal(_("No map found, exiting."))

# debug output (users can use g.gisenv to enable/disable)
# debug level is 1 to 5 (5 is most detailed)
# debug message should not be translated
gs.debug(f"Our calculated value is: {value}."), 3)
```

Do not use the `print` function for informational output. This is reserved for
standard tool output if it has one.

### Developing GRASS Addons

To streamline the development of a GRASS addon in python, you can use [this
template](https://github.com/OSGeo/grass-addon-cookiecutter) powered by
Cookiecutter.

#### Copyright header

Use the following header in your source code.

```python
##############################################################################
# MODULE:    r.foo
#
# AUTHOR(S): John Doe <jdoe at somewhere org>
#
# PURPOSE:   Provide short description of module here...
#
# COPYRIGHT: (C) 2024 by John Doe and the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.
##############################################################################
```

#### Use Standard Options in Interface

GRASS tools must use the GRASS parser to handle its command line parameters. To
make writing parameters simpler and the interfaces more unified, use standard
options. See
[Parser standard options](https://grass.osgeo.org/grass-devel/manuals/parser_standard_options.html).
For example, use this:

```python
# %option G_OPT_V_INPUT
# %end
# %option G_OPT_R_OUTPUT
# %end
```

If needed, override values which need to be different:

```python
# %option G_OPT_V_INPUT
# % key: point_input
# % label: Name of input vector map with points
# % description: Points used for sampling the raster input
# %end
# %option G_OPT_R_OUTPUT
# % key: raster_input
# % label: Name of sampled raster map
# % description: Raster map which will be sampled by the points
# %end
```

Do not repeat the values when a standard option defines them.

#### Consider both Flags and Options to modify behavior

Flags are boolean options that default to false. Their names
are only one character. They are defined using:

```python
# %flag
# % key: n
# % description: Consider zeros to be null values
# %end
```

On the command line, the flag is used with dash as `-n`. In Python, the flag
would be used in the _flags_ parameter of `run_command`:

```python
gs.run_command(..., flags="n", ...)
```

However, options are often better because they improve readability, clarify the
default behavior, and allow for extension of the interface.

**Example:** Consider a tool which by default produces human-readable plain
text output. Then you add JSON output which is
enabled by a flag `j`. Later, you decide to add YAML output. This now needs to
be flag `y` which needs to be exclusive with flag `j`. Soon, you have several
related flags each exclusive with all the others. Using an option instead of a
flag from the beginning allows the interface to accommodate more formats. In
this example, an option named `format` can have default value `plain` and `json`
for JSON output. When you later add YAML, you simply add `yaml` to the possible
values without a need for additional options or flags. The interface definition
for the example would look like:

```python
# %option
# % key: format
# % type: string
# % required: yes
# % options: plain,json,yaml
# % label: Output format
# % descriptions: plain;Plain text output;json;JSON output;yaml;YAML output
# % answer: plain
# %end
```

#### Adding description and keywords

Each tool needs to have a description and at least 3 keywords:

<!-- markdownlint-disable line-length -->

```python
# %module
# % label: Generates a raster map using gaussian random number generator.
# % description: Mean and standard deviation of gaussian deviates can be expressed by the user.
# % keyword: raster
# % keyword: surface
# % keyword: random
# %end
```

<!-- markdownlint-enable line-length -->

Notes:

- the **first** keyword is the tool family which goes to the
  [tool family index](https://grass.osgeo.org/grass-devel/manuals/general.html)
  in the manual and should correspond to the first part of the tool name
  (e.g., r is for raster).
- the **second** keyword is the overall topic which goes to the
  [topic index](https://grass.osgeo.org/grass-devel/manuals/topics.html) in the
  manual
- the **third** (and more) keyword goes to the
  [keyword index](https://grass.osgeo.org/grass-devel/manuals/keywords.html) in
  the manual

These index manual pages are autogenerated during the build process of GRASS
GIS.

#### Lazy import of optional dependencies

A tool may use a package that is not [required](../../REQUIREMENTS.md)
by GRASS GIS and may not be available on a user's system.
In these cases, import only after the _gs.parser_ call. In that way the
tool can be safely compiled even if the dependency is not installed.

```python
def main():
    options, flags = gs.parser()
    try:
        import pandas as pd  # noqa: E402
    except ModuleNotFoundError:
      gs.fatal(_("Pandas library not installed"))
```

#### Tool name

Try to use names which describe shortly the intended purpose of the tool.

The first letters for the tool name should be:

```text
d.    - display tools
db.   - database tools
g.    - general GIS management tools
i.    - imagery tools
m.    - miscellaneous tool tools
ps.   - postscript tools
r.    - raster tools
r3.   - raster3D tools
v.    - vector tools
t.    - temporal tools
g.gui - GUI tools
```

Some additional naming conventions

- specialized export tools: (type).out.(format) eg: _r.out.arc_, _v.out.ascii_
- specialized import tools: (type).in.(format) eg: _r.in.arc_, _v.in.ascii_
- conversion tools: (type).to.(type) eg: _r.to.vect_, _v.to.rast_,
  _r3.to.rast_

Avoid tool names with more than two dots in the name. Example: instead of
_r.to.rast3.elev_ use _r.to.rast3elev_.

#### Data processing history

Tools should record processing history to the output data. For vectors:

```python
gs.vector_history(output)
```

For rasters:

```python
gs.raster_history(output, overwrite=True)
```

### Developing GRASS GUI

Follow
[wxPython style guide](https://wiki.wxpython.org/wxPython%20Style%20Guide).

Please use the following docstring template:

```py
"""!
@package dir.example

@brief Short example package description

Classes:
 - example::ExampleClass

(C) 2024 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author First Author <first somewhere.com>
@author Second Author <second somewhere.com>
@author Some Other <third somewhere.com> (some particular change)
"""
```

#### Translations

To enable [translating of messages](https://weblate.osgeo.org/projects/grass-gis/)
to other languages, use full strings, e.g. (good example):

```python
if ...:
    win.SetLabel(_("Name for new 3D raster map to create"))
else:
    win.SetLabel(_("Name for new raster map to create"))
```

instead of constructing string from several parts (bad example):

```python
# don't do this
if ...:
    maplabel = 'raster map'
else:
    maplabel = '3D raster map'
win.SetLabel(_("Name for new {} to create").format(maplabel))
```

Sometimes the string might have different translation depending on the context
(is it a verb or a noun? matching ending of a word for particular gender; etc).
To help translators, it is suggested to add a comment explaining the context of
string. The comment must start with GTC keyword and must be on a line before
string:

```python
self.bwizard = wx.Button(...,
    # GTC New location
    label = _("N&ew"))

# GTC %s will be replaced with name of current shell
gs.message(_("Running through {}").format(shellname))
```

### Developing C tools

Refer to the [online GRASS Programmer's
Manual](https://grass.osgeo.org/programming8/) or generate it with `make
htmldocs` or `make pdfdocs`.

#### Use GRASS library functions

Use the GRASS library functions, when available, instead of the standard C
functions. The reason for this is that the following functions ensure good
programming practice (e.g. always checking if memory was allocated) and/or
improves portability.

- Memory management: `G_malloc()`, `G_calloc()`, `G_realloc()`, `G_free()`
- Environmental variables: `G_getenv()`, `G_setenv()`, `G_unsetenv()`
- File seek: `G_fseek()`, `G_ftell()`
- Printing: `G_asprintf()`, `G_vsaprintf()`, `G_vfaprintf()`, ...

Please refer to the [programmers manual](https://grass.osgeo.org/programming8/)
for the proper use (e.g., determining if any casts are needed for arguments or
return values) of these library functions. They may perform a task slightly
different from their corresponding C library function, and thus, their use may
not be the same.

#### Returning value of main function

Tool exit status is defined as `EXIT_SUCCESS` or `EXIT_FAILURE` (declared in
`stdlib.h`), e.g.

```c
    {
      ...
      if (G_parser (argc, argv))
          exit (EXIT_FAILURE);

      ...
      exit (EXIT_SUCCESS);
    }
```

#### Messages and data output

See rules for [messages in Python scripts](#messages) for proper usage of
`G_fatal_error()`, `G_warning()`, etc. Message output is not expected to be sent
to pipe or file.

For data output redirected to pipe or file, please use `fprintf()` and specify
the stdout stream as follows:

```c
      fprintf(stdout, ...);
      fflush(stdout);

      fflush(stdout) /* always required when using fprintf(stdout, ...). */
```

#### Header section

Add a header section to file main.c of your tool and make sure you include the
copyright. If you are modifying an existing file you may under no circumstances
remove prior copyright or licensing text that is not your own, even for a major
rewrite. If any original code or code that is in part derived from another's
original work remains, it must be properly cited.

```c
/****************************************************************************
 *
 * MODULE:       g.foo
 * AUTHOR(S):    John Doe <jdoe at somewhere org>
 * PURPOSE:      Provide short description of module here...
 * COPYRIGHT:    (C) 2010 by John Doe, and the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the COPYING file that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
```
