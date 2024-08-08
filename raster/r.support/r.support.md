## DESCRIPTION

**r.support** allows the user to create and/or edit raster map support
information. Editing of raster map color tables, category labels,
header, history, semantic label elements and title is supported.
Category labels can also be copied from another raster map.

### Raster band management

Raster semantic label concept is similar to dimension name in other GIS
and remote sensing applications. Most common usage will be assigning a
remote sensing platform sensor band ID to the raster, although any
identifier is supported. Raster semantic label is suggested to work with
imagery classification tools.\

## EXAMPLES

These examples are based on the North Carolina dataset, more
specifically the *landuse* raster map. Copy the landuse map to the
current mapset

```
g.copy raster=landuse,my_landuse
```

### Update statistics

```
r.support -s map=my_landuse
```

### Update Title

```
r.support map=my_landuse title="Landuse copied"
```

### Append to History Metadata

```
r.support map=my_landuse history="Copied from PERMANENT mapset"
```

### Update Units Display

```
r.support map=my_landuse units=meter
```

### Set semantic label

Note: landuse map doesn\'t confirm to CORINE specification. This is an
example only.

```
r.support map=my_landuse semantic_label=CORINE_LULC
```

## NOTES

If metadata options such as **title** or **history** are given the
module will run non-interactively. If only the map name is given
*r.support* will run interactively within a terminal shell and the user
with be prompted for input.

Freeform metadata information is stored in a \"`hist`\" file which may
be appended to by using the **history** option. Currently this is
limited to 50 lines of text with a maximum line length of 78 characters.
Any input larger than this will be wrapped to the next line. All other
metadata strings available as standard options are limited to 79
characters.

## SEE ALSO

*[r.category](r.category.html), [r.describe](r.describe.html),
[r.info](r.info.html), [r.null](r.null.html), [r.region](r.region.html),
[r.report](r.report.html), [r.timestamp](r.timestamp.html)*

## AUTHORS

Micharl Shapiro, CERL: Original author\
[Brad Douglas](MAILTO:rez@touchofmadness.com): GRASS 6 Port\
M. Hamish Bowman: command line enhancements\
Markus Neteler: category copy from other map\
Maris Nartiss: semantic label management
