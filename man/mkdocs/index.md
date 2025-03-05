---
authors:
    - Corey T. White
    - GRASS Development Team
title: Getting Started
---

## What is GRASS?

[GRASS](https://grass.osgeo.org/) is a geosptial processing engine for
advance analysis and visualization of geospatial data. It is a powerful tool for
processing and analyzing geospatial data sets. GRASS is a free and open source
software, released under an open source [GNU GPLed](https://www.gnu.org/licenses/gpl.html).

Downloaded and installed GRASS here.

<!-- markdownlint-disable-next-line MD013 -->
[:material-download: Download and Install](https://grass.osgeo.org/download/){ .md-button }[Grow GRASS :octicons-heart-fill-24:](https://opencollective.com/grass/contribute){ .md-button .gs-support-button}

## Interfaces

GRASS provides a number of [interfaces](interfaces_overview.md) for interacting
with the software. The most common interfaces are:

- [Command line](terminalintro.md), also know as terminal or shell
- [Python](pythonintro.md)
- [Jupyter Notebooks](jupyterintro.md)
- [Graphical user interface](helptext.md)

## Processing Tools

GRASS provide a wide range of tools for geospatial processing, modeling,
analysis, and visualization. The tools are organized into categories based
on the type of data they process. The tools are prefixed with a letter to
indicate the type of data they process.

The following table provides a list of the prefixes and the categories they represent.
Follow the topics links to see a complete list of tools in each category. Or try
using the search feature to find a specific tool or topic by subject
matter (e.g. hydrology, landscapes).

| Prefix | Category                         | Description                        | Topic                                      |
|--------|----------------------------------|------------------------------------|-------------------------------------------|
| `g.`   | General                          | General GIS management tools       | [General Tools](general.md)               |
| `r.`   | Raster                           | Raster data processing tools       | [Raster Tools](raster.md)                 |
| `r3.`  | 3D Raster                        | 3D Raster data processing tools    | [3D Raster Tools](raster3d.md)            |
| `v.`   | Vector                           | Vector data processing tools       | [Vector Tools](vector.md)                 |
| `i.`   | Imagery                          | Imagery processing tools           | [Imagery Tools](imagery.md)               |
| `t.`   | Temporal                         | Temporal data processing tools     | [Temporal Tools](temporal.md)             |
| `db.`  | Database                         | Database management tools          | [Database Tools](database.md)             |
| `d.`   | Display                          | Display and visualization tools    | [Display Tools](display.md)               |
| `m.`   | Miscellaneous                    | Miscellaneous tools                | [Miscellaneous Tools](miscellaneous.md)   |
| `ps.`  | Postscript                       | Postscript tools                   | [Postscript Tools](postscript.md)         |

## Development

GRASS is an open source project and welcomes contributions from the community.
The GRASS Development documentation provides information on how to get started
developing for GRASS, how to contribute to the project, and how to get involved
with the GRASS community.

For more information on developing for GRASS, see the
[Development Introduction](developmentintro.md) page.
