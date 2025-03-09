## DESCRIPTION

*g.setproj* allows the user to create the PROJ_INFO and the PROJ_UNITS
files to record the coordinate reference system (CRS) information
associated with a current project (previously called location).

## NOTES

The user running *g.setproj* must own the PERMANENT mapset and it must
be currently selected. It is highly recommended to run *g.setproj* after
creating a new project so that conversion programs (such as *v.proj*)
can be run.

The user will be prompted for the projection name. Most projections are
supported. The [PROJ](https://proj.org/) abbreviations for the names are
used with two exceptions, viz. 'll', for latitude / longitude geographic
co-ordinates, and 'stp', for the State Plane Co-ordinate system (used in
the USA).

After the projection name, the user will be asked for a geodetic datum.
If no datum transformation support is needed, the question may be
answered with no, and no datum will be specified in the PROJ_INFO file.
If this is the case the user must specify the ellipsoid (model of the
curvature of the earth) to be used, otherwise it is determined by the
datum being used.

If the datum or ellipsoid required are not listed within this program,
the user/administrator may add the definition to the files datum.table,
datumtransform.table and ellipse.table in the `$GISBASE/etc/proj`
directory.

Depending on the projection selected, the user will then be prompted for
the various other parameters required to define it.

The projections of aea, lcc, merc, leae, leac, and tmerc will generate a
request to the user for the prime meridian and standard parallel for the
output map.

## SEE ALSO

*[g.proj](g.proj.md), [m.proj](m.proj.md), [r.proj](r.proj.md),
[v.proj](v.proj.md), [PROJ](https://proj.org) site*

Further reading:

- A guide to [Map
  Projections](https://web.archive.org/web/20080513234144/http://erg.usgs.gov/isb/pubs/MapProjections/projections.html)
  by USGS
- [ASPRS Grids and
  Datum](https://www.asprs.org/asprs-publications/grids-and-datums)
- [MapRef - The Collection of Map Projections and Reference Systems for
  Europe](https://mapref.org)
- [Projections Transform List](http://geotiff.maptools.org/proj_list/)
  (PROJ)

## AUTHORS

Irina Kosinovsky, U.S. Army Construction Engineering Research
Laboratory  
Morten Hulden, morten at untamo.net - rewrote module and added 121
projections  
Andreas Lange, andreas.lange at rhein-main.de - added prelimnary map
datum support
