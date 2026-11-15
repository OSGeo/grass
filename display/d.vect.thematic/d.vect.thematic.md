## DESCRIPTION

*d.vect.thematic* draws thematic choropleth vector maps based on an
attribute column or an expression involving several columns. It takes a
list of class **breaks** (excluding the minimum and maximum values) and
a list of **colors** to apply to the classes (has to be the number of
class breaks + 1).

Instead of a list of class breaks, the user can also chose a
classification **algorithm** and a number of classes (**nbclasses**).
See the *[v.class](v.class.md)* for more information on these different
algorithms.

## NOTES

The **-l** flag instructs the module to print legend information in
vector legend format as described in *[d.legend.vect](d.legend.vect.md)*
to standard output for further use in graphical software. When combined
with the verbose flag, the legend information will be extended with some
additional statistical information. If the **-n** flag is set, the
module will only print the legend information without drawing the map.

Option **legendfile**, is deprecated, instead use the GRASS_LEGEND_FILE
environmental variable (see *[d.legend.vect](d.legend.vect.md)*) to save
legend into a file. Flag **-e** is deprecated, instead use verbose flag.

## EXAMPLES

### Thematic map with classes

```sh
d.vect.thematic -l map=communes3 column=pop \
  breaks=111393.250000,222785.500000,334177.750000 \
  colors="255:0:0,0:255:0,0:0:255,0,0,0"
```

### Thematic map with calculated class breaks

The following example uses a calculated attribute (`density = pop/area`)
and the standard deviation algorithm to calculate class breaks for 5
classes:

```sh
d.vect.thematic -l map=communes2 column=pop/area algorithm=std \
  nbclasses=5 colors="0:0:255,50:100:255,255:100:50,255:0:0,156:0:0"
```

### Thematic map with legend

Example for the North Carolina sample dataset, colorizing basin polygons
by average elevation and displaying school capacity:

```sh
# create watersheds from elevation map
g.region raster=elevation
r.watershed elevation=elevation threshold=10000 basin=basins_10k

# convert raster to vector
r.to.vect input=basins_10k output=basins_10k type=area column=basin_num

# upload raster statistics to each polygon in vector map
v.rast.stats map=basins_10k raster=elevation column_prefix=elev

# open a graphical display
d.mon wx0

# draw thematic polygons and specify legend title
d.vect.thematic map=basins_10k column=elev_average algorithm=int \
  nclasses=5 colors=0:195:176,39:255:0,251:253:0,242:127:11,193:126:60 \
  legend_title="Average elevation (m)"

# draw thematic points and specify legend title
d.vect.thematic map=schools_wake@PERMANENT column=CORECAPACI algorithm=std \
  nclasses=3 colors=149:203:255,45:143:240,0:81:161 icon=basic/circle size=15 \
  legend_title="School capacity"

# and finally draw legend
d.legend.vect -b at=2,80 font=Sans symbol_size=25
```

![d_vect_thematic example](d_vect_thematic.png)  
*Thematic map of average elevation and school capacity*

## SEE ALSO

*[v.class](v.class.md), [d.legend.vect](d.legend.vect.md),
[d.vect](d.vect.md), [d.graph](d.graph.md), [v.univar](v.univar.md)*

Check also Python module from AddOns:
*[d.vect.thematic2](https://grass.osgeo.org/grass8/manuals/addons/d.vect.thematic2.html)*

## AUTHOR

Moritz Lennert
