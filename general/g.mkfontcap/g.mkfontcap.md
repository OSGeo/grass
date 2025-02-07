## DESCRIPTION

*g.mkfontcap* is a utility to generate a GRASS font configuration file
("fontcap") containing details of the fonts available on the current
system. If [Freetype](https://freetype.org/) is not installed, the font
list will be limited to the set of Hershey stroke fonts supplied with
GRASS. With Freetype enabled however, the module will recursively scan
all files within a predefined hierarchy to find Freetype-compatible
scalable fonts. The list of directories scanned is currently:

```sh
    /usr/lib/X11/fonts
    /usr/share/X11/fonts
    /usr/share/fonts
    /usr/local/share/fonts
    ${HOME}/Library/Fonts
    /Library/Fonts
    /System/Library/Fonts
    ${WINDIR}/Fonts
```

These correspond to directories where fonts can be found on some common
operating systems. Extra directories to search can easily by added using
the **extradirs** parameter, which accepts a comma-separated list. An
extra directory may optionally contain an environment variable *at the
start* of the string, if enclosed in ${xxx} syntax (see examples
above).

The module will normally write to the standard fontcap file location,
`$GISBASE/etc/fontcap`. If the environment variable `GRASS_FONT_CAP` is
set, the output will instead be written to the file specified by that
variable. This is useful if you don't have permission to modify
`$GISBASE/etc/fontcap`: in this case you can use e.g.

```sh
# use local file version instead of system copy
GRASS_FONT_CAP=$HOME/.gfontcap
export GRASS_FONT_CAP

g.mkfontcap
```

to create a personal copy and then to make GRASS use that file instead
of the system copy.

The output list of fonts is sorted first by type (Stroke fonts first,
followed by Freetype) and within each type by the short name of the
font.

## SEE ALSO

*[d.font](d.font.md)*

## AUTHOR

Paul Kelly
