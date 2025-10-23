## DESCRIPTION

The *t.vect.list* module provides the same functionality as
[t.rast.list](t.rast.list.md), the only difference is the vector map
layer metadata.

## EXAMPLE

### Default query

This example show a relative space time vector dataset with the first
three maps with a range of years, instead the last five are valid only
for one year:

```sh
t.vect.list shoreline@shoreline
name|layer|mapset|start_time|end_time
shoreline_1849_1873|None|shoreline|1849|1873
shoreline_1925_1946|None|shoreline|1925|1946
shoreline_1970_1988|None|shoreline|1970|1988
shoreline_1997|None|shoreline|1997|None
shoreline_1998|None|shoreline|1998|None
shoreline_2003|None|shoreline|2003|None
shoreline_2004|None|shoreline|2004|None
shoreline_2009|None|shoreline|2009|None
```

### Using method option

Method option is able to show vector in different way. By default *cols*
value is used, the value *deltagaps* will print the delta between maps
and also the gaps if they exist (like in this example).

```sh
t.vect.list method=deltagaps input=shoreline
id|name|layer|mapset|start_time|end_time|interval_length|distance_from_begin
shoreline_1849_1873@shoreline|shoreline_1849_1873|None|shoreline|1849|1873|24|0
None|None|None|None|1873|1925|52|24
shoreline_1925_1946@shoreline|shoreline_1925_1946|None|shoreline|1925|1946|21|76
None|None|None|None|1946|1970|24|97
shoreline_1970_1988@shoreline|shoreline_1970_1988|None|shoreline|1970|1988|18|121
None|None|None|None|1988|1997|9|139
shoreline_1997@shoreline|shoreline_1997|None|shoreline|1997|None|None|148
None|None|None|None|1997|1998|1|148
shoreline_1998@shoreline|shoreline_1998|None|shoreline|1998|None|None|149
None|None|None|None|1998|2003|5|149
shoreline_2003@shoreline|shoreline_2003|None|shoreline|2003|None|None|154
None|None|None|None|2003|2004|1|154
shoreline_2004@shoreline|shoreline_2004|None|shoreline|2004|None|None|155
None|None|None|None|2004|2009|5|155
shoreline_2009@shoreline|shoreline_2009|None|shoreline|2009|None|None|160
```

## SEE ALSO

*[g.list](g.list.md), [t.create](t.create.md), [t.info](t.info.md),
[t.list](t.list.md), [t.rast.list](t.rast.list.md),
[t.rast3d.list](t.rast3d.list.md)*

## AUTHOR

Sören Gebbert
