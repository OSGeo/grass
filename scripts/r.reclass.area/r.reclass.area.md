## DESCRIPTION

*r.reclass.area* removes areas (pixel clumps) from a raster map
that are smaller than a user specified **lower** area size and/or
greater than a user specified **upper** theshold (both in hectares).

*r.reclass.area* provides two **method**s for filtering:

- *reclass* - raster based filtering
- *rmarea* - vector based filtering

With the *reclass* method, the input raster map is clumped and
then reclassified using the clump size. With the **-m** flag,
*reclass* method will remove small areas already during clumping,
using the *minsize* option there. If the **-c** flag is used,
*r.reclass.area* will skip the creation of a clumped raster and
assume that the input raster map is already clumped.

The *rmarea* method converts the raster map to vector and removes
areas outside the given thresholds using [v.clean](v.clean.md) and
[v.edit](v.edit.md). With the **-v** flag the **output** is the
filtered vector map instead of a raster.

## EXAMPLES

In this example, the ZIP code map in the North Carolina sample dataset
is filtered for large areas (removing smaller areas from the map).

```sh
g.region raster=zipcodes -p
r.report zipcodes unit=h
```

Extract only areas greater than 2000 ha, NULL otherwise:

```sh
r.reclass.area input=zipcodes output=zipcodes_larger2000ha greater=2000

r.report zipcodes_larger2000ha unit=h
```

![Figure: r.reclass.area method=reclass](zipcodes_larger2000ha.png)  
*Figure: r.reclass.area method=reclass*

In this example, the ZIP code map in the North Carolina sample dataset
is filtered for smaller areas which are substituted with the value of
the respective adjacent area with largest shared boundary. Reclass by
substitutional removing of areas smaller than 1000 ha:

```sh
r.reclass.area input=zipcodes output=zipcodes_minor1000ha lesser=1000 method=rmarea
```

![Figure: r.reclass.area method=rmarea](zipcodes_minor1000ha.png)  
*Figure: r.reclass.area method=rmarea*

## SEE ALSO

*[r.clump](r.clump.md), [r.reclass](r.reclass.md),
[r.stats](r.stats.md), [v.to.rast](v.to.rast.md),
[v.clean](v.clean.md), [v.edit](v.edit.md)*

## AUTHORS

NRCS,  
Markus Neteler
Stefan Blumentath
