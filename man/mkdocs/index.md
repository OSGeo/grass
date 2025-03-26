---
authors:
    - Corey T. White
    - GRASS Development Team
title: Getting Started
---

## What is GRASS?

[GRASS](https://grass.osgeo.org/) is a geospatial processing engine for
advance analysis and visualization of geospatial data. GRASS is a free and open source
software, released under the [GNU GPL](https://www.gnu.org/licenses/gpl.html).

<!-- markdownlint-disable-next-line MD013 -->
[:material-download: Download GRASS](https://grass.osgeo.org/download/){ .md-button }[Grow GRASS :octicons-heart-fill-24:](https://opencollective.com/grass/contribute){ .md-button .gs-support-button}

## Interfaces

GRASS provides a number of [interfaces](interfaces_overview.md) for interacting
with the software. The most common interfaces are:

- [Command line](command_line_intro.md), also know as terminal or shell
- [Python](python_intro.md)
- [Jupyter Notebooks](jupyter_intro.md)
- [Graphical user interface](helptext.md)

## Tools

GRASS provides a wide range of tools for geospatial processing, modeling,
analysis, and visualization. The tools are organized into categories based
on the type of data they process. The tools are prefixed with a letter or combination
of letters and numbers to indicate the type of data they process.

The following table provides a list of the prefixes and the categories they represent.
Follow the links to see a complete list of tools in each category. Or try
using the search feature to find a specific tool or topic by subject
matter (e.g. hydrology, landscape).

| Prefix | Category                          | Description                        |
|--------|-----------------------------------|------------------------------------|
| `g.`   | [General](general.md)             | General GIS management tools       |
| `r.`   | [Raster](raster.md)               | Raster data processing tools       |
| `r3.`  | [3D raster](raster3d.md)          | 3D Raster data processing tools    |
| `v.`   | [Vector](vector.md)               | Vector data processing tools       |
| `i.`   | [Imagery](imagery.md)             | Imagery processing tools           |
| `t.`   | [Temporal](temporal.md)           | Temporal data processing tools     |
| `db.`  | [Database](database.md)           | Database management tools          |
| `d.`   | [Display](display.md)             | Display and visualization tools    |
| `m.`   | [Miscellaneous](miscellaneous.md) | Miscellaneous tools                |
| `ps.`  | [PostScript](postscript.md)       | PostScript tools                   |

## Development

GRASS is an open source project and welcomes contributions from the community.
The GRASS Development documentation provides information on how to get started
developing for GRASS, how to contribute to the project, and how to get involved
with the GRASS community.

For more information on developing for GRASS, see the
[Development Introduction](development_intro.md) page.
