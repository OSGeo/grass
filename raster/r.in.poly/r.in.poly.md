## DESCRIPTION

*r.in.poly* allows the creation of GRASS binary raster maps from ASCII
files in the current directory containing polygon, linear, and point
features.

The **input** file is an ASCII text file containing the polygon, linear,
and point feature definitions. The format of this file is described in
the *INPUT FORMAT* section below.

The number of raster **rows** to hold in memory is per default 4096.
This parameter allows users with less memory (or more) on their system
to control how much memory *r.in.poly* uses. Usually the default value
is fine.

## NOTES

The data will be imported using the current region settings to set the
new raster map's bounds and resolution. Any features falling outside the
current region will be cropped. The region settings are controlled with
the *g.region* module.

The format is a simplified version of the standard GRASS vector ASCII
format used by *v.in.ascii*.

Polygons are filled, i.e. they define an area.

### Input Format

The input format for the **input** file consists of sections describing
either polygonal areas, linear features, or point features. The basic
format is:

```sh
A                      <for polygonal areas>
    easting northing
    .
    .
    .
=   cat# label
L                      <for linear features>
    easting northing
    .
    .
    .
=   cat# label
P                      <for single cell point features>
    easting northing
=   cat# label
```

The `A` signals the beginning of a filled polygon. It must appear in the
first column. The `L` signals the beginning of a linear feature. It also
must appear in the first column. The `P` signals the beginning of a
single cell point feature. Again, it must appear in the first column.
The coordinates of the vertices of the polygon, or the coordinates
defining the linear or point feature follow and must have a space in the
first column and at least one space between the *easting* and the
*northing.* To give meaning to the features, the "`=`" indicates that
the feature currently being processed has category value *cat#* (which
must be an integer) and a *label* (which may be more than one word, or
which may be omitted).

## EXAMPLE

An area described by four points:

```sh
A
  591316.80   4926455.50
  591410.25   4926482.40
  591434.60   4926393.60
  591341.20   4926368.70
= 42 stadium
```

## SEE ALSO

*[r.colors](r.colors.md), [d.rast.edit](d.rast.edit.md),
[g.region](g.region.md), [r.in.xyz](r.in.xyz.md), [r.patch](r.patch.md),
[v.in.ascii](v.in.ascii.md), [wxGUI vector digitizer](wxGUI.vdigit.md)*

## AUTHOR

Michael Shapiro, U.S.Army Construction Engineering Research Laboratory
