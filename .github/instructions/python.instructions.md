---
applyTo: "**/*.py"
---
# GRASS Programming Style Guide for Python

## Code Style and Formatting

### Python

- We follow the PEP8 style guide for Python.
- Docstrings follow [PEP257] and use Sphinx style
- We use the following tools in the continuous integration to ensure compliance
with PEP8, consistent formatting, and readable and error-free code:
  - Flake8 tool to ensure compliance with PEP8
  - Ruff linter and code formatter to ensure code quality and consistent code formatting
  - Pylint static code analyser to find bad code practices or errors
- Note that while the entire GRASS code base is formatted with `ruff format`,
full compliance with PEP8, Flake8, and Pylint practices is still work in progress.
- We use [pre-commit](pre-commit.instructions.md) for pre-commit setup and
usage to simplify performing of these checks.

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
use the _RegionManager_ context manager.
This makes any changes done in the tool local for the tool without influencing
other tools running in the same session.

```python
with gs.RegionManager(n=226000, s=222000, w=634000, e=638000):
    stats = gs.parse_command("r.univar", map="elevation", format="json")
```

or

```python
with gs.RegionManager():
    gs.run_command("g.region", n=226000, s=222000, w=634000, e=638000)
    stats = gs.parse_command("r.univar", map="elevation", format="json")
```

If different subprocesses need different regions, use different environments:

```python
with gs.RegionManager(raster=input_raster, env=os.environ.copy()) as manager:
    gs.run_command("r.slope.aspect", elevation=input_raster, slope=slope, env=manager.env)
```

This approach makes the computational region completely safe for parallel
processes as each subprocess has its own environment.

If you can't use a context manager, you can use `gs.use_temp_region()` and
`gs.del_temp_region()`:

```python
gs.use_temp_region()  # From now on, use a separate region in the script.
# Set the computational region with g.region as needed.
grass.run_command('g.region', raster='input')
gs.del_temp_region()
# Original region applies now.
```

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
# SPDX-License-Identifier: GPL-2.0-or-later
##############################################################################
```

#### Use Standard Options in Interface

- GRASS tools must use the GRASS parser to handle its command line parameters.
- To make writing parameters simpler and the interfaces more unified, use standard
options.

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

- the **first** keyword is the tool family which goes to the tool family index
in the manual and should correspond to the first part of the tool name
(e.g., r is for raster).
- the **second** keyword is the overall topic which goes to the topic index
in the manual.
- the **third** (and more) keyword goes to the keyword index in the manual.

These index manual pages are autogenerated during the build process of GRASS.

#### Lazy import of optional dependencies

A tool may use a package that is not [required](./requirements.instructions.md)
by GRASS and may not be available on a user's system.
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

SPDX-License-Identifier: GPL-2.0-or-later

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
