## DESCRIPTION

*r.distance* locates the closest points between "objects" in two raster
maps. An "object" is defined as all the grid cells that have the same
category number, and closest means having the shortest "straight-line"
distance. The cell centers are considered for the distance calculation
(two adjacent grid cells have the distance between their cell centers).

The output is an ascii list, one line per pair of objects, in the
following form:

```sh
from_category:to_category:distance:from_easting:from_northing:to_easting:to_northing
```

**from_category**  
Category number from map1

**to_category**  
Category number from map2

**distance**  
The distance in meters between "from_category" and "to_category"

**from_easting,from_northing**  
The coordinates of the grid cell "from_category" which is closest to "to_category"

**to_easting,to_northing**  
The coordinates of the grid cell "to_category" which is closest to "from_category"

### Flags

**-l** The -l flag outputs the category labels of the matched raster
objects at the beginning of the line, if they exist.

**-o** The -o flag reports zero distance if the input rasters are
overlapping.

## NOTES

The output format lends itself to filtering. For example, to "see" lines
connecting each of the category pairs in two maps, filter the output
using awk and then into *d.graph*:

```sh
r.distance map=map1,map2 | \
  awk -F: '{print "move",$4,$5,"\ndraw",$6,$7}' | d.graph -m
```

To create a vector map of all the "map1" coordinates, filter the output
into awk and then into *v.in.ascii*:

```sh
r.distance map=map1,map2 | \
  awk -F: '{print $4,$5}' | v.in.ascii format=point output=name separator=space
```

## EXAMPLES

### Using JSON output with Pandas to locate closest points

```python
import pandas as pd
import grass.script as gs

result = gs.parse_command(
    "r.distance", map=["zipcodes", "lakes"], flags="l", format="json"
)
df = pd.json_normalize(result)
print(df)
```

Possible output:

```text
        distance  from_cell.category  from_cell.easting  from_cell.northing from_cell.label  to_cell.category  to_cell.easting  to_cell.northing to_cell.label
0   11158.870911               27511             632605              223295            CARY             34300           640585            215495      Dam/Weir
1    1037.304198               27511             631735              222695            CARY             39000           632315            221835     Lake/Pond
2    2277.059507               27511             630015              221605            CARY             43600           630765            219455     Reservoir
..           ...                 ...                ...                 ...             ...               ...              ...               ...           ...
36   4922.600939               27610             642975              219815         RALEIGH             34300           640615            215495      Dam/Weir
37     50.000000               27610             642655              222765         RALEIGH             39000           642705            222765     Lake/Pond
38  11368.069317               27610             642305              220945         RALEIGH             43600           631035            219455     Reservoir
```

## SEE ALSO

*[r.buffer](r.buffer.md), [r.cost](r.cost.md), [r.drain](r.drain.md),
[r.grow](r.grow.md), [r.grow.distance](r.grow.distance.md),
[v.distance](v.distance.md)*

## AUTHOR

Michael Shapiro, U.S. Army Construction Engineering Research Laboratory
