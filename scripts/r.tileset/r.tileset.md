## DESCRIPTION

*r.tileset* generates sets of tiles in another projection that cover a
region in this projection with adequate resolution. By default the
current region and its resolution are used, the bounds and resolution of
another region can be used via the region option.

## NOTES

*r.tileset* does not make "optimal" tilings (as few tiles of the largest
size less than the maximums). This means that from latitude longitude
projection to an appropriate projection for a region, in the degenerate
case, it may create tiles demanding up to twice the necessary
information. Furthermore, generating a tiling near a divergant point of
a source projection, such as the poles of a cylindrical source
projections, results in divergence of the tile set.

Not generating "optimal" tilings may have another consequence; the
aspect ratio of cells in the destination region will not necessarily
carry over to the source region and generated tiles may have cells of
strange aspect ratios. This might be a problem for some map request
services presenting data in an inappropriate projection or with strict
constraints on cell aspect ratio.

## OUTPUT FORMAT

Each tile is listed on a separate line in the output. The lines are
formatted as follows:

```sh
5|125|45|175|80|100
```

This is the default output format. It is the tile's minimum x
coordinate, minimum y coordinate, maximum x coordinate, maximum y
coordinate, width in cells, and height in cells separated by the "\|"
character. The fields can be separated by a different character by
changing the fs option.

```sh
w=5;s=125;e=45;n=175;cols=80;rows=100;
```

This is output in a format convenient for setting variables in a shell
script.

```sh
bbox=5,125,45,175&width=80&height=100
```

This is output in a format convenient for requesting data from some http
services.

## EXAMPLES

Generates tiles in latitude longitude that cover the current region,
each tile will be less than 1024 cells high and 2048 cells across. The
bounds and sizes of tiles in the output are separated by \| (pipe):

```sh
r.tileset sourceproj=+init=epsg:4326 maxrows=1024 maxcols=2048
```

Generates tiles in latitude longitude projection that cover the named
region "ne-rio". The tiles will have 2 cells of overlap. The output
format will be strings like the bbox requests for WMS servers:

```sh
r.tileset sourceproj=+init=epsg:4326 overlap=2 -w region=ne-rio
```

Generates tiles in the coordinate reference system of the project
"IrishGrid". Each tile will be less than 300x400 cells in size, with 3
cells of overlap in the top and right sides of each tile. The output is
in a format where each line is in shell script style. The substitution
`` `g.proj -j project=IrishGrid` `` will only work in a unix style
shell:

```sh
r.tileset sourceproj="`g.proj -j project=IrishGrid`" maxrows=400 maxcols=300 overlap=3 -g
```

## KNOWN ISSUES

- *r.tileset* does not know about meridians that "wrap-around" in
  projections.

## AUTHORS

Cedric Shock  
Updated for GRASS 7 by Martin Landa, CTU in Prague, Czech Republic
