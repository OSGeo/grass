## DESCRIPTION

*r.buffer.lowmem* creates a new raster map showing buffer (a.k.a.
"distance" or "proximity") zones around all cells that contain non-NULL
category values in an existing raster map. The distances of buffer zones
from cells with non-zero category values are user-chosen.

This is the low-memory alternative to the classic
*[r.buffer](r.buffer.md)* module. It is much slower than the classic
version, but will run on massive raster maps without using a lot of RAM.
If your raster map is larger than 32000x32000 cells on a system with 1
GB of RAM, or larger than 90000x90000 cells on a system with 8 GB of
RAM, consider using this module.

For more info see manual of *[r.buffer](r.buffer.md)*.

## EXAMPLE

In the following example, the buffer zones would be (in the default
units of meters): 0-100, 101-200, 201-300, 301-400 and 401-500.

```sh
r.buffer.lowmem input=roads output=roads.buf distances=100,200,300,400,500
```

Result:

```sh
r.category input=roads.buf

      1       distances calculated from these locations
      2       0-100 meters
      3       100-200 meters
      4       200-300 meters
      5       300-400 meters
      6       400-500 meters
```

## SEE ALSO

*[g.region](g.region.md), [r.buffer](r.buffer.md), [r.cost](r.cost.md),
[r.grow.distance](r.grow.distance.md), [r.mapcalc](r.mapcalc.md),
[r.reclass](r.reclass.md), [v.buffer](v.buffer.md)*

## AUTHORS

Michael Shapiro, U.S. Army Construction Engineering Research
Laboratory  
James Westervelt, U.S. Army Construction Engineering Research
Laboratory  
Low-memory Python version by Glynn Clements
