## DESCRIPTION

*r.li.patchdensity* calculates the "patch density index", the number of
patches per square kilometer, as:

```sh
PD = Npatch / A
```

with:

- **A**:sampling area size
- **Npatch**: number of patches

This index is calculated using a 4 neighbour algorithm, diagonal cells
are ignored when tracing a patch.

## NOTES

Do not use absolute path names for the **config** and **output**
file/map parameters. If the "moving window" method was selected in
**g.gui.rlisetup**, then the output will be a raster map, otherwise an
ASCII file will be generated in the folder
`C:\Users\userxy\AppData\Roaming\GRASS8\r.li\output\` (MS-Windows) or
`$HOME/.grass8/r.li/output/` (GNU/Linux).

A sample area of only NULL values is considered to have zero patches,
that is, the result is always ≥ 0.

## EXAMPLES

To calculate patch density index on map *my_map*, using *my_conf*
configuration file (previously defined with *g.gui.rlisetup*) and saving
results in *my_out*, run:

```sh
r.li.patchdensity input=my_map conf=my_conf output=my_out
```

Example for Spearfish forest areas:

```sh
g.region raster=landcover.30m -p
# extract forested areas:
r.category landcover.30m
r.mapcalc "forests = if(landcover.30m >= 41 && landcover.30m <= 43, 1, null())"

# patch density (7x7 moving window defined in g.gui.rlisetup):
r.li.patchdensity forests conf=movwindow7 out=forests_p_dens7
r.univar forests_p_dens7
d.rast.leg forests_p_dens7

r.to.vect forests out=forests feature=area
d.vect forests type=boundary
```

Forest map (North Carolina sample dataset) example:

```sh
g.region raster=landclass96 -p
r.mapcalc "forests = if(landclass96 == 5, 1, null() )"
r.li.patchdensity input=forests conf=movwindow7 out=forests_patchdensity_mov7

# verify
r.univar forests_patchdensity_mov7
r.to.vect input=forests output=forests type=area
d.mon wx0
d.rast forests_patchdensity_mov7
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

Michael Shapiro - CERL (patch identification)  
Markus Metz (statistics)
