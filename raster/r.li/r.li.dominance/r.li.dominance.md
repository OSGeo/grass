## DESCRIPTION

*r.li.dominance* calculates the "dominance diversity index" as:  
![rliDominance formula](r_li_dominance_formula.png)  
with:

- *i*: patch type
- *m*: number of different patch types
- *p_i*:
  proportional abundance of patch type *i*

## NOTES

Do not use absolute path names for the **config** and **output**
file/map parameters. If the "moving window" method was selected in
**g.gui.rlisetup**, then the output will be a raster map, otherwise an
ASCII file will be generated in the folder
`C:\Users\userxy\AppData\Roaming\GRASS8\r.li\output\` (MS-Windows) or
`$HOME/.grass8/r.li/output/` (GNU/Linux).

If the input raster map contains only NULL values then *r.li.dominance*
returns NULL.

## EXAMPLES

To calculate the dominance diversity index on map *my_map*, using
*my_conf* configuration file (previously defined with *g.gui.rlisetup*)
and saving results in *my_out*, run:

```sh
r.li.dominance input=my_map conf=my_conf output=my_out
```

Forest map (Spearfish sample dataset) example:

```sh
g.region raster=landcover.30m -p
r.mapcalc "forests = if(landcover.30m >= 41 && landcover.30m <= 43,1,null())"
r.li.dominance input=forests conf=movwindow7 out=forests_dominance_mov7
r.univar forests_dominance_mov7
```

Forest map (North Carolina sample dataset) example:

```sh
g.region raster=landclass96 -p
r.mapcalc "forests = if(landclass96 == 5, 1, null() )"
r.li.dominance input=forests conf=movwindow7 out=forests_dominance_mov7

# verify
r.univar forests_dominance_mov7
r.to.vect input=forests output=forests type=area
d.mon wx0
d.rast forests_dominance_mov7
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
