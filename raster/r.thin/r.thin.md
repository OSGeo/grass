## DESCRIPTION

*r.thin* scans the named *input* raster map layer and thins non-NULL
cells that denote linear features into linear features having a single
cell width. Raster lines often need to be thinned (skeletonizing raster
features) to a single pixel width before they can be transformed to
vector data.

*r.thin* will thin only the non-NULL (no data) raster cells of the named
*input* raster map layer within the current geographic region settings.
The cell width of the thinned *output* raster map layer will be equal to
the cell resolution of the currently set geographic region. All of the
thinned linear features will have the width of a single cell.

*r.thin* will create a new *output* raster data file containing the
thinned linear features. *r.thin* assumes that linear features are
encoded with positive values on a background of NULL's in the *input*
raster data file, hence it creates a NULL/1 output map.

## NOTES

*r.thin* only creates raster map layers. In order to create a vector
map, the user will need to run *[r.to.vect](r.to.vect.md)* on the
resultant raster map.

*r.thin* may create small spurs or "dangling lines" during the thinning
process. These spurs may be removed (after creating a vector map layer)
by *[v.clean](v.clean.md)* (*rmdangle* tool).

This code implements the thinning algorithm described in "Analysis of
Thinning Algorithms Using Mathematical Morphology" by Ben-Kwei Jang and
Ronlad T. Chin in *Transactions on Pattern Analysis and Machine
Intelligence*, vol. 12, No. 6, June 1990. The definition Jang and Chin
give of the thinning process is "successive removal of outer layers of
pixels from an object while retaining any pixels whose removal would
alter the connectivity or shorten the legs of the skeleton."

The skeleton is finally thinned when the thinning process converges;
i.e., "no further pixels can be removed without altering the
connectivity or shortening the skeleton legs" (p. 541). The authors
prove that the thinning process described always converges and produces
one-pixel thick skeletons. The number of iterations depends on the
original thickness of the object. Each iteration peels off the outside
pixels from the object. Therefore, if the object is \<= n pixels thick,
the algorithm should converge in \<= iterations.

## EXAMPLE

To vectorize the raster map *streams_derived* in the North Carolina
sample dataset that represents the stream network derived from the 10m
resolution DEM by *r.watershed*, run:

```sh
g.region raster=elevation -p
# create flow accumulation map
r.watershed elevation=elevation accumulation=accum_50K thresh=50000
# extract streams from flow accumulation map
r.mapcalc "streams_from_flow = if(abs(accum_50K) > 1000, 1, null())"

# skeletonize map
r.thin streams_from_flow out=streams_thin

d.mon wx0
d.rast streams_from_flow
d.erase
d.rast streams_thin
```

![Raster feature thinning (skeletonizing)](r_thin_network.png)  
Raster feature thinning (skeletonizing)

The resulting map cabe optionally vectorized:

```sh
r.to.vect streams_thin output=streams_thin type=line
# visualize
d.rast accum_50K
d.vect streams_thin color=red width=2
```

![Vectorized stream network after thinning extracted from flow
accumulation map](r_thin_vectorized.png)  
Vectorized stream network after thinning extracted from flow
accumulation map

## SEE ALSO

*[g.region](g.region.md), [r.to.vect](r.to.vect.md),
[v.clean](v.clean.md), [wxGUI vector digitizer](wxGUI.vdigit.md),
[v.build](v.build.md)*

## AUTHORS

Olga Waupotitsch, U.S.Army Construction Engineering Research Laboratory

The code for finding the bounding box as well as input/output code was
written by Mike Baba (DBA Systems, 1990) and Jean Ezell (USACERL, 1988).
