## DESCRIPTION

This program allows a user to convert coordinates from one projection to
another. Coordinates can be read from one file, converted, and results
written to another file. Alternatively, if the **input=-**, eastings and
northings may be passed to the program directly from standard input. If
the **output** option is omitted, the results are sent directly to
standard output. In this way *m.proj* can be used as a simple frontend
to the [PROJ](https://proj.org/apps/cs2cs.html) *cs2cs* utility. The
**-i** or **-o** flags make the task especially easy for the common
problem of converting to or from lat/long WGS84.

*Note*: This program does not transform GRASS maps, it is designed to
determine the equivalent coordinate values of an individual position or
list of positions. Use *[v.proj](v.proj.md)* to reproject vector maps or
*[r.proj](r.proj.md)* for raster maps.

For an introduction to map projections (with PROJ),see the manual page
of [r.proj](r.proj.md).

## NOTES

*cs2cs* expects input data to formatted as `x y`, so if working with
latitude-longitude data be sure to send the `x` value first, i.e.,
`longitude latitude`. Output data will be exported using the same
convention.

*cs2cs* will treat a third data column as a `z` value (elevation) and
will modify the value accordingly. This usually translates into small
but real differences in that data column.

*cs2cs* does not expect the input stream to contain column headings,
only numbers. If your data file has lines you wish to have passed
through without being processed, they must start with the '`#`'
character.

If sending *m.proj* data from standard input, be aware that the data is
first stored to a temporary file before being processed with *cs2cs*. It
is therefore not advisable to send *m.proj* data from an open data
stream. The module will stop listening for incoming data after 2 seconds
of inactivity. You may use the projection parameters gleaned from
*m.proj*'s verbose mode (**--verbose**) with *cs2cs* directly in this
case.

Custom projection parameters can be used via the **proj_in** and
**proj_out** options. Full documentation of the projection parameter
format may be found on the [PROJ](https://proj.org) website. Using these
options will fully override the default parameters the module would
normally use.

By using the **--verbose** verbose flag, the user can see exactly what
projection parameters will be used in the conversion as well as some
other informative messages.

If output is to lat/long, it will be formatted using PROJ's
Degree:Minute:Second (DMS) convention of `DDDdMM'SSS.SS"H`. This can be
handy if you wish to quickly convert lat/long decimal degree data into
its DMS equivalent.  
Alternatively, to have *m.proj* output data in decimal degrees, use the
**-d** flag. This flag can also be used with non-lat/long data to force
a higher number of decimal places (the *cs2cs* default is 2).

Note that Lat/long output can be converted to GRASS's DMS convention
(`DDD:MM:SSS.SSSH`) by piping the results of *m.proj* through the *sed*
stream editor as follows.

```sh
m.proj -o ... | sed -e 's/d/:/g' -e "s/'/:/g"  -e 's/"//g'
```

## EXAMPLES

The examples are suitable for the North Carolina sample dataset if not
stated otherwise:

### Reproject vector point coordinate pairs to Long/Lat WGS84

The *m.proj* module is designed to work seamlessly with point data
exported from the GIS with *[v.out.ascii](v.out.ascii.md)*, as the
following example shows.

```sh
# Long/Lat WGS84 output in DMS
v.out.ascii bridges | m.proj -o input=-

# Long/Lat WGS84 output in decimal degree
v.out.ascii bridges | m.proj -o -d input=-
```

### Reproject Long/Lat WGS84 coordinate pair to current map projection

To convert a Long/Lat WGS84 coordinate pair to the current map CRS using
the **-i** flag which sets the target projection parameters
automatically from the current project definition:

```sh
echo "-78.61168178 33.92225767" | m.proj -i input=-
645513.47|19180.31|0.00
```

The same, but load points from a file named `waypoints.txt` and continue
on to import the results into a GRASS vector points map in the current
map projection:

```sh
# check file content
cat waypoints.txt
-78.43977824 33.89587173
-78.54944691 33.88964566
-78.51078074 33.88141495
-77.14037951 35.60543020

# reproject points and generate vector map on the fly
m.proj -i input=waypoints.txt | v.in.ascii input=- output=test_pnts

# verify result
v.db.select test_pnts cat|dbl_1|dbl_2|dbl_3
1|661427.74|16329.14|0
2|651285.43|15586.79|0
3|654867.21|14690.64|0
4|778074.58|207402.6|0
```

### Custom projection parameter usage

To transform points from a UTM projection (here specified with detailed
projection definition rather than using an EPSG code) into the
Gauss-Krüger Grid System, importing from and exporting to files:

```sh
m.proj proj_in="+proj=utm +name=utm +a=6378137.0 +es=0.006694380 \
    +zone=32 +unfact=1.0" proj_out="+proj=tmerc +name=tmerc \
    +a=6377397.155 +es=0.0066743720 +lat_0=0.0 +lon_0=9.0 +k=1.0 \
    +x_0=3500000.0" input=utm.coord.txt output=new.gk.coord.txt
```

Projection parameters provided in the above case: `+proj` (projection
type), `+name` (projection name), `+a` (ellipsoid: equatorial radius),
`+es` (ellipsoid: eccentricity squared), `+zone` (zone for the area),
`+unfact` (conversion factor from meters to other units, e.g. feet),
`+lat_0` (standard parallel), `+lon_0` (central meridian), `+k` (scale
factor) and `+x_0` (false easting). Sometimes false northing is needed
which is coded as `+y_0`. Internally, the underlying
[PROJ](https://proj.org) projection library performs an inverse
projection to latitude-longitude and then projects the coordinate list
to the target projection.

Datum conversions are automatically handled by the PROJ library if
`+datum` settings are specified on **both** the input **and** output
projections on the command line. The `+towgs84` parameter can be used to
define either 3 or 7 term datum transform coefficients, satisfying this
requirement.

If a datum is specified there is no need for the `+ellps=` or underlying
parameters, `+a=`, `+es=`, etc.

Another custom parameter usage example:

```sh
m.proj proj_in="+proj=tmerc +datum=ire65 +lat_0=53.5 +lon_0=-8 +x_0=200000 \
    +y_0=250000 +k=1.000035" proj_out="+proj=ll +datum=wgs84" input=wpt.txt
```

or without datum transformation:

```sh
m.proj proj_in="+proj=tmerc +ellps=modif_airy +lat_0=53.5 +lon_0=-8 +x_0=200000 \
    +y_0=250000 +k=1.000035" proj_out="+proj=ll +datum=wgs84" input=wpt.txt
```

In this example no datum transformation will take place as a datum was
not specified for the input projection. The datum specified for the
output projection will thus be silently ignored and may be left out; all
that is achieved a simple conversion from projected to geodetic
co-ordinates, keeping the same datum (and thus also the same ellipsoid).

For more usage examples, see the documentation for the
[PROJ](https://proj.org) *cs2cs* program.

## REFERENCES

- Evenden, G.I. (1990) [Cartographic projection procedures for the UNIX
  environment - a user's
  manual](https://pubs.usgs.gov/of/1990/of90-284/ofr90-284.pdf). USGS
  Open-File Report 90-284 (OF90-284.pdf) See also there: Interim Report
  and 2nd Interim Report on Release 4, Evenden 1994).
- [PROJ](https://proj.org) Cartographic Projection Library

## SEE ALSO

*[g.proj](g.proj.md), [r.proj](r.proj.md), [v.proj](v.proj.md),
[i.rectify](i.rectify.md), [v.in.ascii](v.in.ascii.md),
[v.out.ascii](v.out.ascii.md)*

## AUTHOR

M. Hamish Bowman, Dept. Marine Science, Otago University, New Zealand  
Functionality inspired by the *m.proj* and *m.proj2* modules for GRASS
GIS 5.
