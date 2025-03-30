# Development and Maintenance Documentation

Here is development and maitanance documentation. The API documentation
is at appropriate places, but here is the information relevant to
contributing to GRASS GIS and its maintanance.

## How to generate the 'Programmer's Manual'

You can locally generate the [GRASS GIS Programmer's Manual](https://grass.osgeo.org/programming8/).

This needs doxygen (<http://www.doxygen.org>) and optionally
Graphviz dot (<http://www.research.att.com/sw/tools/graphviz/>).

To build the GRASS programmer's documentation, run

```sh
make htmldocs
```

Or to generate documentation as a single html file
(recommended for simple reading)

```sh
make htmldocs-single
```

This takes quite some time. The result is in `lib/html/index.html`
which refers to further document repositories in

```text
lib/vector/html/index.html
lib/db/html/index.html
lib/gis/html/index.html
```

The master file is: `./grasslib.dox` where all sub-documents have to
be linked to.

To generate the documents in PDF format, run

```sh
make pdfdocs
```
