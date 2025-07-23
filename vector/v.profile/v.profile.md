## DESCRIPTION

*v.profile* prints out distance and attributes of points/lines along a
profiling line. Distance is calculated from the first profiling line
coordinate pair or from the beginning of vector line.  
The *buffer* (tolerance) parameter sets how far point can be located
from a profiling line and still be included in the output data set.  
The *output* map option can be used to visually check which points are
profiled. The *buffer* (tolerance) setting does not affect lines. Lines
are sampled at their crossing point with profiling line.

By default Z values are printed if input vector is a 3D map. It can be
disabled with the **-z** flag.  
The profiling line can be provided as N,E coordinate pairs or from an
input vector map. As a profiling line must be a single line, the user
should use the *profile_where* parameter to select a single line from a
profile input map if it contains multiple vector features.

## NOTES

Currently the module can profile only points and lines (including 3D
ones). Areas and other complex features are not supported. If in future
users can provide reasonable examples how area sampling should work and
why it is important, area (or any other feature type) sampling can be
added.

Due to bugs in GRASS native buffering algorithms, this module for now
depends on GEOS and will not function if GRASS is compiled without GEOS.
This restriction will be removed as soon as GRASS native buffer
generation is fixed.

## EXAMPLES

List all geonames along part of road NC-96 (NC Basic dataset). The
output will be stored in a file for later usage. We will use comma as
delimiter and three numbers after decimal separator for distance. Output
file will contain data for all points, that are within 500 m range to
profiling line.

```sh
v.profile input=geonames@PERMANENT output=/home/user/NC_96_geonames.csv\
  separator=comma dp=3 buffer=500 profile_map=roadsmajor@PERMANENT profile_where=cat=56

# Now lets see the output:
cat NC_96_geonames.csv
Number,Distance,cat,GEONAMEID,NAME,ASCIINAME,ALTERNATEN,FEATURECLA,FEATURECOD,COUNTRYCOD,CC2,ADMIN1,POPULATION,ELEVATION,GTOPO30,TIMEZONE,MODIFICATI,PPLKEY,SRC_ID,MAINT_ID
1,360.719,26881,4482019,"New Zebulon Elementary School","New Zebulon Elementary School","","S","SCH","US","","NC",0,106,91,"America/Iqaluit","2006-01-15 00:00:00",0,0,0
2,846.806,22026,4476596,"Little River, Township of","Little River, Township of","","A","ADMD","US","","NC",0,0,91,"America/Iqaluit","2006-01-15 00:00:00",0,0,0
3,2027.918,16681,4470608,"Hendricks Pond","Hendricks Pond","","H","RSV","US","","NC",0,0,91,"America/Iqaluit","2006-01-15 00:00:00",0,0,0
4,2027.918,16690,4470622,"Hendricks Dam","Hendricks Dam","","S","DAM","US","","NC",0,0,91,"America/Iqaluit","2006-01-15 00:00:00",0,0,0
5,2999.214,39338,4496159,"Union Chapel","Union Chapel","","","","US","","NC",0,0,96,"America/Iqaluit","2006-01-15 00:00:00",0,0,0
6,3784.992,43034,4500325,"Zebulon Airport","Zebulon Airport","","S","AIRP","US","","NC",0,108,98,"America/Iqaluit","2006-01-15 00:00:00",0,0,0
```

Create river valley crossection and provide river marker (Spearfish
dataset):

```sh
# Take elevation samples
r.profile input=elevation.dem@PERMANENT output=/home/user/elevation.profile \
  profile=600570.27364,4920613.41838,600348.034348,4920840.38617

# Now get distance to place where river marker should be set
v.profile input=streams@PERMANENT output=/home/user/river_profile.csv \
  east_north=600570.27364,4920613.41838,600348.034348,4920840.38617
```

## BUGS

Strings are enclosed in double quotes ", still quotes within string are
not escaped.

Output does not contain Vector CAT values. Only way how to get CAT value
is from the attribute table.

If sampled feature (point, line) contains multiple attribute entries
(has multiple CAT values), only the first one is reported. If this is a
limitation in some practical use case, a feature request in GRASS GIS
issue tracker should be opened.

## SEE ALSO

*[r.profile](r.profile.md), [Linear Referencing System](lrs.md)*

## AUTHOR

Maris Nartiss
