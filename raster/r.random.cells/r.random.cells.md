## DESCRIPTION

*r.random.cells* generates a random sets of raster cells that are at
least **distance** apart. The cells are numbered from 1 to the numbers
of cells generated, all other cells are NULL (no data). Random cells
will not be generated in areas masked off.

### Detailed parameter description

**output**  
Random cells. Each random cell has a unique non-zero cell value ranging
from 1 to the number of cells generated. The heuristic for this
algorithm is to randomly pick cells until there are no cells outside of
the chosen cell's buffer of radius **distance**.

**distance**  
Determines the minimum distance the centers of the random cells will be
apart.

**seed**  
Specifies the random seed that *r.random.cells* will use to generate the
cells. If the random seed is not given, *r.random.cells* will get a seed
from the process ID number.

## NOTES

The original purpose for this program was to generate independent random
samples of cells in a study area. The **distance** value is the amount
of spatial autocorrelation for the map being studied.

## EXAMPLES

### Random cells in given distances

North Carolina sample dataset example:

```sh
g.region n=228500 s=215000 w=630000 e=645000 res=100 -p
r.random.cells output=random_500m distance=500
```

### Limited number of random points

Here is another example where we will create given number of vector
points with the given minimal distances. Let's star with setting the
region (we use large cells here):

```sh
g.region raster=elevation
g.region rows=20 cols=20 -p
```

Then we generate random cells and we limit their count to 20:

```sh
r.random.cells output=random_cells distance=1500 ncells=20 seed=200
```

Finally, we convert the raster cells to points using
*[r.to.vect](r.to.vect.md)* module:

```sh
r.to.vect input=random_cells output=random_points type=point
```

An example of the result is at the Figure below on the left in
comparison with the result without the cell limit on the right.

Additionally, we can use *[v.perturb](v.perturb.md)* module to add
random spatial deviation to their position so that they are not
perfectly aligned with the grid. We cannot perturb the points too much,
otherwise we might seriously break the minimal distance we set earlier.

```sh
v.perturb input=random_points output=random_points_moved parameters=50 seed=200
```

In the above examples, we were using fixed seed. This is advantageous
when we want to generate (pseudo) random data, but we want to get
reproducible results at the same time.

![Cells and points filling the space](r_random_cells.png)

*Figure: Generated cells with limited number of cells (upper left),
derived vector points (lower left), cells without a count limit (upper
right) and corresponding vector points (lower right)*

## REFERENCES

Random Field Software for GRASS GIS by Chuck Ehlschlaeger

As part of my dissertation, I put together several programs that help
GRASS (4.1 and beyond) develop uncertainty models of spatial data. I
hope you find it useful and dependable. The following papers might
clarify their use:

- Ehlschlaeger, C.R., Shortridge, A.M., Goodchild, M.F., 1997.
  Visualizing spatial data uncertainty using animation. Computers &
  Geosciences 23, 387-395. doi:10.1016/S0098-3004(97)00005-8
- [Modeling Uncertainty in Elevation Data for Geographical
  Analysis](http://www.geo.hunter.cuny.edu/~chuck/paper.html), by
  Charles R. Ehlschlaeger, and Ashton M. Shortridge. Proceedings of the
  7th International Symposium on Spatial Data Handling, Delft,
  Netherlands, August 1996.
- [Dealing with Uncertainty in Categorical Coverage Maps: Defining,
  Visualizing, and Managing Data
  Errors](http://www.geo.hunter.cuny.edu/~chuck/acm/paper.html), by
  Charles Ehlschlaeger and Michael Goodchild. Proceedings, Workshop on
  Geographic Information Systems at the Conference on Information and
  Knowledge Management, Gaithersburg MD, 1994.
- [Uncertainty in Spatial Data: Defining, Visualizing, and Managing Data
  Errors](http://www.geo.hunter.cuny.edu/~chuck/gislis/gislis.html), by
  Charles Ehlschlaeger and Michael Goodchild. Proceedings, GIS/LIS'94,
  pp. 246-253, Phoenix AZ, 1994.

## SEE ALSO

*[r.random.surface](r.random.surface.md), [r.random](r.random.md),
[v.random](v.random.md), [r.to.vect](r.to.vect.md),
[v.perturb](v.perturb.md)*

## AUTHOR

Charles Ehlschlaeger; National Center for Geographic Information and
Analysis, University of California, Santa Barbara
