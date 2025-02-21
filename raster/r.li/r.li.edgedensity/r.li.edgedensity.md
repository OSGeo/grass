## DESCRIPTION

*r.li.edgedensity* calculates:

- the density of all edges of patch type *k* ![r.li.edgedensity formula
  1](r_li_edgedensity_formula_1.png) , or
- the density of all edges in the sampling area if *k* is not specified,
  ![r.li.edgedensity formula 2](r_li_edgedensity_formula_2.png)

with:

- **k**: patch type
- **m**: number of patch types
- **n**: number of edge segments of patch type k
- **e_ik** :total edge length in the landscape involving patch type *k*
- **Area**: total landscape area

The unit is meters per hectare.

## NOTES

Do not use absolute path names for the **config** and **output**
file/map parameters. If the "moving window" method was selected in
**g.gui.rlisetup**, then the output will be a raster map, otherwise an
ASCII file will be generated in the folder
`C:\Users\userxy\AppData\Roaming\GRASS8\r.li\output\` (MS-Windows) or
`$HOME/.grass8/r.li/output/` (GNU/Linux).

If the input raster map contains only NULL values then
*r.li.edgedensity* consider to have 0 patches.  
If area is 0 *r.li.edgedensity* returns NULL; this is only possible if
input raster is masked.

## EXAMPLES

To calculate the edge density index on map *my_map*, using *my_conf*
configuration file (previously defined with *g.gui.rlisetup*) and saving
results in *my_out*, run:

```sh
r.li.edgedensity input=my_map conf=my_conf output=my_out
```

To calculate edge density index of patch_type 34, using "my_conf"
configuration file and on map "my_map", saving results in "my_out" file
run:  

```sh
r.li.edgedensity input=my_map conf=my_conf output=my_out patch_type=34
```

Forest map (Spearfish sample dataset) example:

```sh
g.region raster=landcover.30m -p
r.mapcalc "forests = if(landcover.30m >= 41 && landcover.30m <= 43,1,null())"
r.li.edgedensity input=forests conf=movwindow7 out=forests_edgedens_mov7
r.univar forests_edgedens_mov7
```

Forest map (North Carolina sample dataset) example:

```sh
g.region raster=landclass96 -p
r.mapcalc "forests = if(landclass96 == 5, 1, null() )"
r.li.edgedensity input=forests conf=movwindow7 out=forests_edgedensity_mov7

# verify
r.univar forests_edgedensity_mov7
r.to.vect input=forests output=forests type=area
d.mon wx0
d.rast forests_edgedensity_mov7
d.vect forests type=boundary
```

## SEE ALSO

*[r.li](r.li.md) (package overview),
[g.gui.rlisetup](g.gui.rlisetup.md)*

## REFERENCES

McGarigal, K., and B. J. Marks. 1995. FRAGSTATS: spatial pattern
analysis program for quantifying landscape structure. USDA For. Serv.
Gen. Tech. Rep. PNW-351. ([PDF](https://doi.org/10.2737/PNW-GTR-351))

## AUTHORS

Serena Pallecchi, student of Computer Science University of Pisa
(Italy).  
Commission from Faunalia Pontedera (PI), Italy (<www.faunalia.it>)  
Markus Metz
