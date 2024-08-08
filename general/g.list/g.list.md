## DESCRIPTION

*g.list* searches for data files matching a pattern given by wildcards
or POSIX Extended Regular Expressions.

## NOTES

The output of *g.list* may be useful for other programs\' parameter
input (e.g. time series for *[r.series](r.series.html)*) when used with
*separator=comma*.

## EXAMPLES

List all raster maps as continuous, sorted list:

```
g.list type=rast
```

List all vector maps as continuous, sorted list with MAPSET info (i.e.
fully-qualified map names):

```
g.list type=vector -m
```

List all raster and vector maps ordered by mapset:

```
g.list type=raster -p
```

List all raster and vector maps as continuous, sorted list:

```
g.list type=rast,vect
```

List all available GRASS data base files:

```
g.list type=all
```

### Mapset search path

If **mapset** is not specified, then *g.list* searches for data files in
the mapsets that are included in the search path (defined by
*[g.mapsets](g.mapsets.html)*). See `g.mapsets -p`.

```
g.list rast -p

raster map(s) available in mapset <user1>:
dmt
...
raster map(s) available in mapset <PERMANENT>:
aspect
...
```

Option **mapset**=. (one dot) lists only data files from the current
mapset:

```
g.list rast mapset=.
...
```

Similarly, **mapset**=\* (one asterisk) prints data files from all
available mapsets also including those that are not listed in the
current search path (see `g.mapsets -l`).

```
g.list rast mapset=* -p

raster map(s) available in mapset <landsat>:
lsat5_1987_10
...
raster map(s) available in mapset <user1>:
dmt
...
raster map(s) available in mapset <PERMANENT>:
aspect
...
```

### Wildcards

List all vector maps starting with letter \"r\":

```
g.list type=vector pattern="r*"
```

List all vector maps starting with letter \"r\" or \"a\":

```
g.list type=vector pattern="[ra]*"
```

List all raster maps starting with \"soil\_\" or \"landuse\_\":

```
g.list type=raster pattern="{soil,landuse}_*"
```

List certain raster maps with one variable character/number:

```
g.list type=raster pattern="N45E00?.meters"
```

Use of **exclude** parameter:

```
# without exclude:
  g.list rast pat="r*" mapset=PERMANENT
  railroads
  roads
  rstrct.areas
  rushmore

# exclude only complete word(s):
  g.list rast pat="r*" exclude=roads mapset=PERMANENT
  railroads
  rstrct.areas
  rushmore

# exclude with wildcard:
  g.list rast pat="r*" exclude="*roads*" mapset=PERMANENT
  rstrct.areas
  rushmore
```

### Regular expressions

List all soil maps starting with \"soils\" in their name:

```
g.list -r type=raster pattern='^soils'
```

List \"tmp\" if \"tmp\" raster map exists:

```
g.list -r type=raster pattern='^tmp$'
```

List \"tmp0\" \...\"tmp9\" if corresponding vector map exists (each map
name linewise):

```
g.list -r type=vector pattern='^tmp[0-9]$'
```

List \"tmp0\"\...\"tmp9\" if corresponding vector map exists (each map
name comma separated):

```
g.list -r type=vector separator=comma pattern='^tmp[0-9]$'
```

### Extended regular expressions

List all precipitation maps for the years 1997-2012, comma separated:

```
g.list -e type=raster separator=comma pattern="precip_total.(199[7-9]|200[0-9]|201[0-2]).sum"
```

### Maps whose region overlaps with a saved region

List all raster maps starting with \"tmp\_\" whose region overlaps with
the region of \"test\" raster map:

```
g.region raster=test save=test_region
g.list type=raster pattern='tmp_*' region=test_region
```

List \"tmp0\"\...\"tmp9\" vector maps whose region overlaps with the
current region:

```
g.list -r type=vector pattern='^tmp[0-9]$' region=.
```

List all raster and vector maps whose region overlaps with the default
region of the PERMANENT mapset in the current project (DEFAULT_WIND):

```
g.list type=rast,vect region=*
```

Note that, without `region=*`, `g.list type=rast,vect` simply lists all
available raster and vector maps from the current search path regardless
of their region.

## SEE ALSO

*[r.series](r.series.html), [t.list](t.list.html),
[t.rast.list](t.rast.list.html), [t.vect.list](t.vect.list.html)*

[Regular expressions](http://en.wikipedia.org/wiki/Regular_expression)
(aka regex) - from Wikipedia, the free encyclopedia

## AUTHOR

Huidae Cho\
grass4u@gmail.com\
based on general/manage/cmd/list.c by Michael Shapiro
