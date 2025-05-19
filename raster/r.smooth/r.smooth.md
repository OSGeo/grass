## DESCRIPTION

The module performs an adaptive (edge preserving) raster smoothing
with anisotropic diffusion.
The module implements three diffusivity (conductance) functions:
exponential and quadratic (as sugessted by Perona & Malik 1990) and
Tukey's biweight (as suggested by Black et al. 1998).

Implementation of the Tukey's biweight diffusivity function is quite
aggressive and tends to produce continuous smoothed areas. Its output is
well suited for raster segmentation. The *-p* flag allows to switch
into a more gentle variation of the function that better preserves minor
details and thus is better suited e.g. for visualisation.

The *threshold* parameter is in map units (DN, meters, etc.)
and serves an indicator on how small difference should be considered as
noise. It will be different for each map.

The *lambda* parameter impacts smoothing speed. For exponential
and quadratic diffusivity functions the value is automatically bound
to range of 0 to 0.25. For the Tukey diffusivity function full range is
available.

The *steps* parameter sets number of iterations for smoothing.
Depending on selected diffusivity function, large number of iterations
can result in a loss of fine details. In terms of detail preservation
functions can be ordered with increasing preservation as *quadratic,
exponential, Tukey, Tukey with the -p flag*.

## NOTES

Internally calculations are performed in double precision and only at the end
converted to the same type as the input map.

The module will try to keep all temporary data in RAM. Thus it is important
to set the *memory* parameter as high as possible.

The original Perona & Malik paper uses von Neumann (4-connected) neighbourhood for
value calculation, but this module uses Moore (8-connected) neighbourhood.
Computed gradients of neighbouring cells are adjusted to equalise distances for
diagonals and non-square cells.

## EXAMPLE

```sh
# Set computational region to orthophoto map
g.region raster=ortho_2001_t792_1m -p

# Smooth with average in a 3x3 window. Note how all edges have became blurry
# but at the same time streets and roofs are still noisy
r.neighbors input=ortho_2001_t792_1m output=ortho_smoothed_avg\
 size=3 method=average

# Smooth with median in a 3x3 window. Although better than average smoothing,
# thin lines still are lost and noise on streets and roofs is still present.
r.neighbors input=ortho_2001_t792_1m output=ortho_smoothed_med\
 size=3 method=median

# Smooth with quadratic diffusivity function. Note better preservation of
# small details and reduction of noise on streets and roofs.
r.smooth function=quadratic input=ortho_2001_t792_1m output=ortho_smoothed_qa\
 threshold=15 lambda=0.4 steps=20

# Smooth with exponential diffusivity function. Even better edge delineation
# but at the same time increase of noise in really noisy areas.
r.smooth function=exponential input=ortho_2001_t792_1m output=ortho_smoothed_ex\
 threshold=15 lambda=0.4 steps=20

# Smooth with aggressive Tukey's diffusivity function. Better preservation of
# minor details e.g. as road markings but little smoothing in areas with
# fine, well expressed texture.
r.smooth function=tukey input=ortho_2001_t792_1m output=ortho_smoothed_ta\
 threshold=15 lambda=0.4 steps=20

# Smooth with preserving Tukey's diffusivity function. Only low noise areas
# have been smoothed.
r.smooth function=tukey input=ortho_2001_t792_1m output=ortho_smoothed_tp\
 threshold=15 lambda=0.4 steps=20 -p
```

## SEE ALSO

* Smooth with statistics: *[r.neighbours](r.neighbours)*
* The Mumford-Shah variational model for image segmentation (an add-on):
*[r.smooth.seg](https://grass.osgeo.org/grass84/manuals/addons/r.smooth.seg.html)*

## REFERENCES

* Perona P. and Malik J. 1990. Scale-space and edge detection using anisotropic
diffusion. *IEEE transactions on pattern analysis and machine intelligence*,
12(7).
* Black M.J., Sapiro G., Marimont D.H. and Heeger D. 1998. Robust anisotropic
diffusion. *IEEE transactions on image processing*, 7(3).

## AUTHOR

Maris Nartiss, University of Latvia.
