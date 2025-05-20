## DESCRIPTION

*r.reclass.area* reclasses a raster map greater or less than a user
specified area size (in hectares).

If the **-c** flag is used, *r.reclass.area* will skip the creation of a
clumped raster and assume that the input raster is already clumped.

## EXAMPLES

In this example, the ZIP code map in the North Carolina sample dataset
is filtered for large areas (removing smaller areas from the map).

```sh
g.region raster=zipcodes -p
r.report zipcodes unit=h
```

Extract only areas greater than 2000 ha, NULL otherwise:

```sh
r.reclass.area input=zipcodes output=zipcodes_larger2000ha mode=greater value=2000

r.report zipcodes_larger2000ha unit=h
```

![Figure: r.reclass.area method=reclass](zipcodes_larger2000ha.png)  
*Figure: r.reclass.area method=reclass*

In this example, the ZIP code map in the North Carolina sample dataset
is filtered for smaller areas which are substituted with the value of
the respective adjacent area with largest shared boundary. Reclass by
substitutional removing of areas minor of 1000 ha:

```sh
r.reclass.area input=zipcodes output=zipcodes_minor1000ha mode=lesser value=1000 method=rmarea
```

![Figure: r.reclass.area method=rmarea](zipcodes_minor1000ha.png)  
*Figure: r.reclass.area method=rmarea*

## SEE ALSO

*[r.reclass](r.reclass.md), [r.clump](r.clump.md),
[r.stats](r.stats.md)*

## AUTHORS

NRCS,  
Markus Neteler
