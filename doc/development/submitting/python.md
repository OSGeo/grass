# Submitting Python Code

When submitting Python code to GRASS GIS GitHub repository
([​https://github.com/OSGeo/grass/pulls](https://github.com/OSGeo/grass/pulls)),
please take care of following rules:

## File structure

### GRASS module

Instructions for the GRASS script parser can be found in the
[​g.parser](https://grass.osgeo.org/grass-stable/manuals/g.parser.html) module's
help page.

Use the directory structure to place your script appropriately into the source
tree: scripts go into `scripts` directory.

Also add a `Makefile` and a `<module>.html` file into this directory. See existing
Python scripts for examples.

Add a header section to the script you submit and make sure you include the
copyright. The purpose section is meant to contain a general over view of the
code in the file to assist other programmers that will need to make changes to
your code. For this purpose use a comment and [​Python docstring](https://www.python.org/dev/peps/pep-0257/).

Example (fictitious header for a script called _g.myscript_):

```py
#!/usr/bin/env python

##############################################################################
# MODULE:    g.myscript
#
# AUTHOR(S): Alice Doe <email AT some domain>
#
# PURPOSE:   Describe your script here from maintainer perspective
#
# COPYRIGHT: (C) 2022 Alice Doe and the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.
##############################################################################

"""Describe your script here from Python user perspective"""
```

The copyright protects your rights according to GNU General Public License
([​https://www.gnu.org/licenses/](https://www.gnu.org/licenses/)).

You can easily autogenerate the header and parameters from an existing module
using the `--script` flag. Example:

```bash
d.rast --script
```

Just select an existing module which is close to your application to save efforts.

### Python library

Files are placed in `lib/python`. This directory becomes a package `grass` after
compilation. Each subdirectory is a subpackage.

## Style

### Indentation

Use 4-space indentation (GNU Emacs python-mode default). **Do not use tabs
(tabulators) at all**. Note that Python determines nesting based upon indentation,
so it is quite crucial to be consistent, i.e. use given rules.

### Automation

Use Black to format new files:

```bash
black python_file.py
```

Use Flake8 to check formatting of all files:

```bash
flake8 --extend-ignore=E203,E266,E501 --max-line-length=88 python_file.py
```

If the file you changed gives too many error in lines you did not change, see
if the directory or any parent directory contains a file called .flake8 which
contains a less strict configuration for this legacy code. Use it with the
`--config` pararameter:

```bash
flake8 --config  {path_to_flake8_file} {path_to_python_file}
```

For example like this:

```bash
flake8 --config lib/python/.flake8 lib/python/temporal/register.py
```

:exclamation:
It is very convenient and recommended to [use pre-commit](./submitting.md#use-pre-commit)
to do both Black formatting and Flake8 file validation.

### Editor settings for 4-space indentation

The correct editor settings for Python indentation

- [​Geany](https://www.geany.org/) editor:
  - Edit > Preferences > Editor > Indentation tab > Type: Spaces
- [​PyCharm](https://www.jetbrains.com/pycharm/) IDE:
  - already fine and includes code-linting
- [​atom](https://atom.io/) IDE:
  - ...
- [​GNU Emacs](https://www.gnu.org/software/emacs/) editor:
  - python-mode default
- [​spyder](https://www.spyder-ide.org/) editor:
  - ...

For the usage of editors for GRASS GIS Python programming, see
[​https://grasswiki.osgeo.org/wiki/Tools\_for\_Python\_programming#PyCharm\_IDE](https://grasswiki.osgeo.org/wiki/Tools_for_Python_programming#PyCharm_IDE)

### PEP8 standard Style

Follow PEP8 standard and use the `pep8` tool to check compliance of your code to
this standard. You can either install it with `pip3` or it is offered for your
operating system as package `python3-pep8` `python3-pycodestyle`.

Update 2020:

- `pep8` has been renamed to `pycodestyle-3`
  - Use of the `pep8` tool will be removed in future, please install and use
    `pycodestyle-3` instead.

Note that not all code is currently compliant to complete PEP8, so we are using
a custom configuration stored in `tools/pep8config.txt`
([​here](https://github.com/OSGeo/grass/blob/master/tools/pep8config.txt) shipped
within the GRASS GIS source code), so use:

```bash
pycodestyle-3 --config=grass_master/tools/pep8config.txt directory_to_check
```

Alternatively, you can use `pycodestyle-3` with `--diff` option to check just
the parts of the code you have changed:

```bash
git diff | pycodestyle-3 --diff --config=grass_master/tools/pep8config.txt
```

The best practice is to use `pycodestyle-3` with default configuration (i.e.,
without custom configuration file) for new files and new code in old files.

Do not fix (intentionally or unintentionally) existing style issues in code
(at lines) you are not changing. If you are fixing style issues, do it in a
separate commit.

Summary of the most important rules:

- Make sure a new line is at the end of each file.

- Use three double quotes for docstrings (`"""..."""`). Use double quotes for
  translatable (user visible) strings, single quotes for the rest.
- Remove trailing whitespace from the end of lines. Empty lines should not contain
  any spaces.
- Put space between operators:

```py
# use this:
angle = angle * pi / 180
# not this:
angle = angle*pi/180
```

- Do not use space around parentheses:

```py
# use this:
grass.run_command('g.region', raster='myrast')
# not this:
grass.run_command( 'g.region', raster='myrast' )
```

- Do not use space around '=' in a keyword argument:

```py
# use this:
grass.run_command('g.region', raster='myrast')
# not this:
grass.run_command( 'g.region', raster = 'myrast' )
```

- Use space after comma:

```py
# use this:
a = [1, 2, 3]
# not this:
a = [1,2,3]
```

- Good practice is to use named parameters in functions:

```py
# use this:
dlg = wx.FileDialog(parent=self, message=_("Choose file to save current workspace"),
                    wildcard=_("GRASS Workspace File (*.gxw)|*.gxw"), style=wx.FD_SAVE)
# not this:
dlg = wx.FileDialog(self, _("Choose file to save current workspace"),
                     _("GRASS Workspace File (*.gxw)|*.gxw"), wx.FD_SAVE)
```

## Writing the code

### Temporary files

Create and use secure temporary files and directories. Use the grass.tempfile()
or grass.tempdir() functions to do this. e.g.

```py
# setup temporary file
TMP = grass.tempfile()
if TMP is None:
    grass.fatal("Unable to create temporary files")
```

TODO: this needs to be fixed, it's more complicated

### Temporary region

If your script needs to modify computational region, use the following functions:

```py
grass.use_temp_region()
# now you can safely modify the region
grass.run_command('g.region', raster='input')
# and when you are done:
grass.del_temp_region()
```

Note that changing computational region is usually not necessary and not even
expected. Typically, user sets region before running the script and expects all
computations to be done within this region.

### Checking inputs of a module

Use grass.findfile() when there is a need to test if a map exists.

```py
# test for input raster map
result = grass.find_file(name=map_name, element='cell', quiet=True)
if not result['file']
  grass.fatal("Raster map <%s> not found" % map_name)

# test for input vector map
result = grass.find_file(name=map_name, element='vector', quiet=True)
if not result['file']
  grass.fatal("Vector map <%s> not found" % map_name)
```

... and so forth. See 'g.manual g.findfile' for details.

### Overwrite maps

Do not use `overwrite=True` when calling a module from a Python script, if to
overwrite or not should be automatically detected based on calling of the script
with `--o` or without.

### Messages

For any informational output, use the grass.message() function. For error
messages should be used grass.fatal\_error() or grass.error() and for warnings
grass.warning(). For debugging purposes grass.debug().

```py
# normal message:
grass.message(_("Done"))

# verbose message:
grass.verbose(_("Computation finished successfully"))

# warning:
grass.warning(_("No input values found, using default values"))

# error:
grass.error(_("No map found"))

# fatal error:
# prints error and exits or raises exception (use set_raise_on_error to set the behavior)
grass.fatal_error("No map found, exiting")

# debug output (use g.gisenv to enable/disable)
# debug level is 1 to 5 (5 is most detailed)
grass.debug(_("Our calculated value is: %d" % value), 3)
```

Do not use the `print` statement (print function in Python 3) for informational
output. This is reserved for standard module output if it has one.

### Translations

To enable translating of messages to other languages (than English), use full
strings, e.g. (good example):

```py
if ...:
    win.SetLabel(_("Name for new 3D raster map to create"))
else:
    win.SetLabel(_("Name for new raster map to create"))
```

instead of constructing string from several parts (bad example):

```py
if ...:
    maplabel = 'raster map'
else:
    maplabel = '3D raster map'
win.SetLabel(_("Name for new %s to create") % maplabel)
```

Sometimes the string might have different translation depending on the context
(is it a verb or a noun? matching ending of a word for particular gender; etc).
To help translators, it is suggested to add a comment explaining the context of
string. The comment must start with GTC keyword and must be on a line before
string:

```py
self.bwizard = wx.Button(...,
    # GTC New location
    label = _("N&ew"))

# GTC %s will be replaced with name of current shell
grass.message(_("Running through %s") % shellname)
```

See also locale/README for more information on translation process and related
issues.

### Adding description and keywords

Each module needs to have a description and at least 3 keywords. Here an example
from scripts/g.extension/g.extension.py:

<!-- markdownlint-disable line-length -->
```py
\# %module
# % label: Maintains GRASS Addons extensions in local GRASS installation.
# % description: Downloads and installs extensions from GRASS Addons repository or other source into the local GRASS installation or removes installed extensions.
# % keyword: general
# % keyword: installation
# % keyword: extensions
# % keyword: addons
# % keyword: download
# %end
```
<!-- markdownlint-enable line-length -->

Notes:

- the **first** keyword is the module family (**g**.list belongs to "general")
  which go to the [module family index](https://grass.osgeo.org/grass74/manuals/general.html)
  in the manual
- the **second** keyword is the overall topic which go to the
  [topic index](https://grass.osgeo.org/grass74/manuals/topics.html) in the manual
- the **third\* (and more) keyword is describing further keywords which go to
  the [keyword index](https://grass.osgeo.org/grass74/manuals/keywords.html) in
  the manual**

These index manual pages are autogenerated during the build process of GRASS GIS.

### Dependencies on external Python libraries

With dependencies on external, non-standard modules should use lazy imports:
[​https://lists.osgeo.org/pipermail/grass-dev/2018-October/090321.html](https://lists.osgeo.org/pipermail/grass-dev/2018-October/090321.html)

## Documentation and comments

Comment your classes and functions with docstrings. Use Sphinx (reStructuredText)
syntax.

Comment also the code itself such as the meaning of variables, conditions etc.

Take the time to add comments throughout your code explaining what the code is
doing. It will save a huge amount of time and frustration for other programmers
that may have to change your code in the future.

## Checking the code

Use tools such as [​pylint](https://www.pylint.org/) and pep8 to check your code
(both style and correctness). Just note that default settings of these tools is
not fully compatible with wxGUI/wxPython style and that some of the reported
errors may not apply to your code.

## Testing

[​https://grass.osgeo.org/grass-stable/manuals/libpython/](https://grass.osgeo.org/grass-stable/manuals/libpython/)

## See also

### Related submitting rules

- [General](./general.md) general GRASS and git instructions
- [Docs](./docs.md) user documentation of modules and GUI
- [wxGUI](./wxGUI.md) wxPython-based GUI has its own rules

### GRASS GIS documentation

- GRASS Programming manual for C API and existing classes:
  [​https://grass.osgeo.org/programming8/](https://grass.osgeo.org/programming8/)
  (add more specific link)
- GRASS scripts in Python: [​https://grasswiki.osgeo.org/wiki/GRASS\_and\_Python](https://grasswiki.osgeo.org/wiki/GRASS_and_Python)
- GRASS scripts in PyGRASS: [​https://grass.osgeo.org/grass-stable/manuals/libpython/](https://grass.osgeo.org/grass-stable/manuals/libpython/)
- Python based wxGUI: [​https://grass.osgeo.org/grass-stable/manuals/wxgui/](https://grass.osgeo.org/grass-stable/manuals/wxgui/)

### External documentation

- Style Guide for Python Code: [​https://peps.python.org/pep-0008/](https://peps.python.org/pep-0008/)
- Python Style Guide by Guido van Rossum:
  [​https://www.python.org/doc/essays/styleguide/](https://www.python.org/doc/essays/styleguide/)
- Additional info on Python docstrings:
  [​https://epydoc.sourceforge.net/docstrings.html](https://epydoc.sourceforge.net/docstrings.html)
