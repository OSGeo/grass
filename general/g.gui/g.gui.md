## DESCRIPTION

The *g.gui* module allows user to start the Graphical User Interface
(GUI) from the command line prompt or to change the default User
Interface (UI) settings.

GRASS GIS comes with both a wxPython-based GUI aka *[wxGUI](wxGUI.html)*
(**ui=wxpython**) and command line text-based UI (**ui=text**).

## NOTES

If the **-d** update flag is given or the `GRASS_GUI` environmental
[variable](variables.html) is unset, then the GRASS internal variable
`GUI` is permanently changed and the selected **ui** will be used as the
default UI from then on.

All GRASS internal variables (see *[g.gisenv](g.gisenv.html)*) are
stored in the user\'s home directory in a hidden file called
`$HOME/.grass8/rc` on Unix-based operating systems and
`%APPDATA%\GRASS8\rc` on MS Windows. Note that these GRASS internal
variables are not the shell environment variables and the `rc` file is
not a classic UNIX run command file, it just contains persistent GRASS
variables.

## EXAMPLES

### Set default user interface settings

Set default user interface setting to command line, text-based UI:

```
g.gui -d ui=text
```

Set default user interface setting to the graphical user interface (GUI)
and *launch* the GUI:

```
g.gui -d ui=wxpython
```

Set default user interface setting to the graphical user interface (GUI)
but *do not launch* the GUI:

```
g.gui -dn ui=wxpython
```

### Load workspace from command line

Start the GUI from command line with an existing workspace:

```
g.gui workspace=myproject.gxw
```

## SEE ALSO

*[wxGUI](wxGUI.html), [g.gisenv](g.gisenv.html), [GRASS
variables](variables.html)*

[wxGUI wiki
page](https://grasswiki.osgeo.org/wiki/WxPython-based_GUI_for_GRASS)

## AUTHORS

Martin Landa, FBK-irst, Trento, Italy\
Hamish Bowman, Otago University, Dunedin, New Zealand (fine tuning)
