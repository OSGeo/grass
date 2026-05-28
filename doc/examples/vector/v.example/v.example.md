## DESCRIPTION

*v.example* is an example vector module that does something like
labeling all vectors with value 1. A new map is written instead of
updating the input map. See the source code for details.

## NOTES

Some more detailed notes go here.

## EXAMPLE

Label all vectors with value 1 (North Carolina sample dataset):

```sh
v.example input=zipcodes_wake output=newmap
v.category newmap option=report
```

## SEE ALSO

*[r.example](r.example.md), [r.category](v.category.md),
[v.example](v.example.md)* *[GRASS Programmer's
Manual](https://grass.osgeo.org/programming8/)*

## AUTHOR

Radim Blazek, ITC-irst, Trento, Italy
