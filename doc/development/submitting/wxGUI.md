# Submitting wxGUI

GUI is divided into components. One component is usually placed in one directory.

Remember that functionality such as generating plots should be primarily
provided by library or modules not GUI.

Try to create create also `g.gui.*` module for the new GUI component. It helps
advanced users to access functionality and developers to test it. Moreover, it
helps to keep components separated and thus, it supports re-usability.

## File structure

Add a header section to each file you submit and make sure you include the
copyright. The purpose section is meant to contain a general over view of the
code in the file to assist other programmers that will need to make changes to
your code. For this purpose use Python docstring.

The copyright protects your rights according to GNU General Public License
(<www.gnu.org>).

Please use the following docstring template:

```py
"""!
@package dir.example

@brief Short example package description

Classes:
 - example::ExampleClass

(C) 2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author First Author <first somewhere.com>
@author Second Author <second somewhere.com>
@author Some Other <third somewhere.com> (some particular change)
"""
```

## Documentation and comments

Some information about docstrings:

- always use """triple double quotes""" around docstrings
- there's no blank line either before or after the docstring
- add information on the parameters used by the function you have to use the
  following format

  ```py
  :param TYPE NAMEPARAM: explanation
  ```

  where TYPE is the type (bool, str, int) of parameter and it is optional;
  NAMEPARAM is the name of parameter

- add information about the return of function using

  ```py
  :return: explanation
  ```

- add reference to classes and function using

  ```py
  :class:`CLASSNAME`
  :func:`FUNCNAME`
  ```

- add TODO in docstring as follow

  ```py
  .. todo::
      information todo
  ```

  in similar way you can add warning and note.

When using class Signal from `grass.pydispatch.signal`, write a short
description to as a constructor parameter and describe implementation details
in standard comments before the definition. Note the signal in each function
which is invoking it.

## Writing the code

Do not use print command unless you know what you are doing.

Use `wx.ID_ANY` instead of `-1`.

Use `GError`, `GWarning` and `GMessage` instead of `wx.MessageBox()`

Do not use `grass.run_command()` or `grass.read_command()`. Use functions and
classes which use threads such as `RunCommand`.

When using `AddGrowableCol` and/or `AddGrowableRow` with sizers, put it after
adding widgets into the sizer, not just after creating of the sizer (needed for
wxPython >= 2.9).

For translatable strings use underscore:

```py
_("User visible text")
```

The underscore function must be explicitly imported:

```py
from core.utils import _
```

## Testing

## See also

### Related submitting rules

- [General notes](./general.md) general GRASS and git instructions
- [Python code](./python.md) Python library and scripts related instructions
  which applies to wxGUI too if not stated otherwise
- [Documentation-related notes](./docs.md) user documentation of modules and GUI

### GRASS GIS documentation

- Sphinx Pages about how to develop wxGUI:
  [​https://grass.osgeo.org/grass-stable/manuals/wxgui/](https://grass.osgeo.org/grass-stable/manuals/wxgui/)
- Wiki Pages about how to develop wxGUI: [​https://grasswiki.osgeo.org/wiki/WxGUI](https://grasswiki.osgeo.org/wiki/WxGUI)
- GRASS Trac wiki has pages about the state of wxGUI development:
  [​https://trac.osgeo.org/grass/wiki/wxGUIDevelopment](https://trac.osgeo.org/grass/wiki/wxGUIDevelopment)
- GRASS Programming manual for C API and existing classes:
  [​https://grass.osgeo.org/programming8/](https://grass.osgeo.org/programming8/)

### External documentation

- wxPython Style Guide: [​https://wiki.wxpython.org/wxPython\_Style\_Guide](https://wiki.wxpython.org/wxPython%20Style%20Guide)
