## DESCRIPTION

*v.split* splits vector lines into shorter segments using a maximal
distance between nodes. The resulting length of all segments is expected
to be equal and not higher than the given **length** parameter.

## NOTES

*v.split* does not change the layer, nor the category information, nor
the attribute table links of the original file. It just splits each line
in segments and attributes the same category to all segments of the same
original line. As the attribute table is linked to the features with
their category as key, all segments originating from the same original
line are linked to the same line in the original attribute table which
is just copied to the new map.

### Notes on individual segment information

When running *v.to.db* on a map produced by *v.split*, *v.to.db* will
add length information for each segment in its respective attribute
line, but since all the segments of the same original line share the
same attribute table line, it only gets updated once.

To obtain the length of each segment, the user will have to attribute
different category values to each of them. The best way to do this on a
separate layer, using *v.category*

```sh
v.category v_split op=add layer=2 output=v_split_2
```

and then run the following commands on the new layer 2:

```sh
v.db.addtable v_split_2 layer=2
v.db.addcolumn map=v_split_2 column="length double precision" layer=2
v.to.db map=v_split_2 type=line option=length columns=length units=meters layer=2
```

To link the new segments in the new layer to the original segments, use:

```sh
v.db.addcolumn map=v_split_2 layer=2 column="cat_1 int"
v.to.db map=v_split_2 layer=2 option=query query_layer=1 query_column=cat columns=cat_1
```

## EXAMPLES

The examples are based on the North Carolina sample data.

### Example 1: Inserting nodes to railroad lines map

```sh
# extract one railroad line for this example
v.extract input=railroads output=myrr cats=1

# show line, category, direction (to find the beginning)
g.region vector=myrr
d.erase
d.vect myrr display=shape,cat,dir

# insert nodes at a distance not longer than 1000m
v.split input=myrr output=myrr_split_1km length=1000

d.vect myrr_split_1km display=shape,topo
```

Note: In case that the vector line data are not polylines, generate
first polylines as the second step, eg.:

```sh
# join segments into polyline
v.build.polylines input=myrr output=myrr_polylines
# regenerate categories
v.category input=myrr_polylines output=myrailroads option=add
```

### Example 2: Inserting vertices to railroad lines map

Note: first run the two steps from example 1.

```sh
# insert vertices at a distance not longer than 1000m
v.split -n input=myrr output=myrr_split length=1000
d.vect myrr_split display=shape,topo
```

## SEE ALSO

*[v.edit](v.edit.md), [v.build.polylines](v.build.polylines.md),
[v.to.points](v.to.points.md), [v.segment](v.segment.md)*

## AUTHOR

Radim Blazek
