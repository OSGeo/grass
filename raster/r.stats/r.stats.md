## DESCRIPTION

*r.stats* calculates the area present in each of the categories or
floating-point intervals of user-selected **input** raster map. Area
statistics are given in units of square meters and/or cell counts. This
analysis uses the current geographic region (*[g.region](g.region.md)*)
and mask settings (*[r.mask](r.mask.md)*). The output statistics can be
saved to a **output** file.

Area statistics is printed in square meters for each category when
**-a** is given. Similarly if **-c** flag is chosen, areas will be
stated also in number of cells.

## NOTES

If a single raster map is specified, a list of categories will be
printed. The **-x** flag will print x and y (column and row) starting
with 1 (both first row and first column are indexed with 1). If multiple
raster maps are specified, a cross-tabulation table for each combination
of categories in the raster maps will be printed.

For example, if one raster map was specified, the output would look
like:

```sh
1 1350000.00
2 4940000.00
3 8870000.00
```

If three raster maps were specified, the output would look like:

```sh
0 0 0 8027500.00
0 1 0 1152500.00
1 0 0 164227500.00
1 0 1 2177500.00
1 1 0 140092500.00
1 1 1 3355000.00
2 0 0 31277500.00
2 0 1 2490000.00
2 1 0 24207500.00
2 1 1 1752500.00
3 0 0 17140000.00
3 1 0 11270000.00
3 1 1 2500.00
```

Within each grouping, the first field represents the category value of
first raster map, the second represents the category values associated
with second raster map, the third represents category values for third
raster map, and the last field gives the area in square meters for the
particular combination of these three raster maps' categories. For
example, above, combination 3,1,1 covered 2500 square meters. Fields are
separated by the **separator** option. The output from *r.stats* is
sorted by category or category intervals (for floating-point raster
maps).

Note that the user has only the option of printing out cell statistics
in terms of cell counts and/or area totals. Users wishing to use
different units than are available here should use
*[r.report](r.report.md)*.

## EXAMPLES

### Report area for each category

Report area for each category in the single raster map:

```sh
g.region raster=geology_30m
r.stats -a in=geology_30m nv=no-data sep=tab

217     71960000.000000
262     19760000.000000
270     67760000.000000
405     25120000.000000
583     2520000.000000
720     480000.000000
766     840000.000000
862     6560000.000000
910     4360000.000000
921     1200000.000000
946     360000.000000
948     80000.000000
no-data 33375200000.000004
```

### Report sorted number of cells for each category

Report sorted number of cells for each category in the single raster map
(suppress NULL data):

```sh
g.region raster=geology_30m
r.stats -cn input=geology_30m sort=desc

217 1799
270 1694
405 628
262 494
862 164
910 109
583 63
921 30
766 21
720 12
946 9
948 2
```

### Report area, number of cells, and percents in multiple raster maps

Report area, number of cells, and percents (separated by tabs) for each
category in multiple raster maps (suppress NULL data):

```sh
g.region raster=towns
r.stats -nacp input=towns,urban separator=tab

1       55      23840000.000000 596     11.89%
2       55      13680000.000000 342     6.82%
3       55      1360000.000000  34      0.68%
4       55      16040000.000000 401     8.00%
5       55      98240000.000000 2456    48.98%
6       55      19760000.000000 494     9.85%
```

### Report sorted area intervals of floating-point raster map

Report sorted area for each interval of floating-point input raster map.
Number of intervals are given by **nsteps** option.

```sh
g.region raster=elevation
r.stats -an input=elevation nsteps=10 sort=desc separator=tab

95.879221-105.954329    36440000.000000
85.804114-95.879221     30800000.000000
105.954329-116.029436   30080000.000000
116.029436-126.104543   27960000.000000
126.104543-136.17965    26440000.000000
136.17965-146.254757    20880000.000000
75.729007-85.804114     15880000.000000
65.6539-75.729007       6040000.000000
146.254757-156.329865   5720000.000000
55.578793-65.6539       760000.000000
```

### Report raster cell counts in multiple raster maps

Report raster cell counts of landuse and geological categories within
zipcode areas:

```sh
g.region raster=zipcodes
# landuse/landcover and zipcodes
r.stats -c input=landclass96,zipcodes separator=comma

# landuse/landcover, geology and zipcodes with category labels
r.stats -c input=landclass96,zipcodes,geology_30m separator=comma -l
```

## SEE ALSO

*[g.region](g.region.md), [r.report](r.report.md), [r.coin](r.coin.md),
[r.describe](r.describe.md), [r.stats.quantile](r.stats.quantile.md),
[r.stats.zonal](r.stats.zonal.md), [r.statistics](r.statistics.md),
[r.univar](r.univar.md)*

## AUTHORS

Michael Shapiro, U.S. Army Construction Engineering Research
Laboratory  
Sort option by Martin Landa, Czech Technical University in Prague, 2013
