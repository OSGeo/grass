## DESCRIPTION

This module performs adaptive, edge-preserving raster smoothing
using anisotropic diffusion. The result is a denoised raster that
retains important features, especially edges. Unlike traditional
smoothing methods (such as a uniform average filter, like the one available
in *r.neighbors*), this module smooths only areas with similar values
and works to preserve sharp transitions between different regions.

![An example of smoothed image](r_smooth_edgepreserve.jpg)  
*Figure: "A" is the original (unsmoothed) raster and
"B" is the smoothed version (with quite agressive settings to emphasize
smoothing effects).*

The module supports three types of diffusivity (conductance) functions:

- **Exponential**
- **Quadratic**  
  *(both as proposed by Perona & Malik, 1990)*
- **Tukey’s biweight**  
  *(as proposed by Black et al., 1998)*

The **Tukey’s biweight** function is more aggressive than the others and
tends to produce large, smoothly blended areas. This makes it
particularly useful for raster segmentation. If you prefer a gentler
effect that preserves smaller features, you can use the **`-p`** flag,
which switches to a softer variant of the Tukey function — ideal for
applications like visualization.

### Parameters

- **`threshold`**  
  Defines how small a difference in raster values should be treated as
  noise. The unit depends on the raster (e.g., digital numbers, meters,
  etc.), and should be adjusted individually for each map.

- **`lambda`**  
  Controls the smoothing rate.
  - For **exponential** and **quadratic** functions, the value is
  automatically clamped between `0` and `0.25` for numeric stability.
  - For **Tukey**, the full range is used.

- **`steps`**  
  Specifies the number of smoothing iterations. Higher values result
  in stronger smoothing.  
  Note that:
  - Excessive iterations can cause loss of fine detail.
  - Detail preservation improves in this order:  
    `quadratic` → `exponential` → `Tukey` → `Tukey` with **`-p`** flag.

## NOTES

Internally calculations are performed in double precision and only at the end
converted to the same type as the input map.

The module will try to keep all temporary data in RAM. Thus it is important
to set the **`memory`** parameter as high as possible.  If the map does
not fit into RAM, a temporary file will be used instead.

The original *Perona & Malik* paper uses von Neumann (4-connected)
neighbourhood for value calculation, but this module uses Moore
(8-connected) neighbourhood. Computed gradients of neighbouring cells
are adjusted to equalise distances for diagonals and non-square cells.

## EXAMPLES

```sh
# Set computational region to orthophoto map
g.region raster=ortho_2001_t792_1m -p

# Smooth with average in a 3x3 window. Note how all edges have became blurry
# but at the same time streets and roofs are still noisy
r.neighbors input=ortho_2001_t792_1m output=ortho_smoothed_avg \
 size=3 method=average

# Smooth with median in a 3x3 window. Although better than average smoothing,
# thin lines still are lost and noise on streets and roofs is still present.
r.neighbors input=ortho_2001_t792_1m output=ortho_smoothed_med \
 size=3 method=median

# Smooth with quadratic diffusivity function. Note better preservation of
# small details and reduction of noise on streets and roofs.
r.smooth.edgepreserve function=quadratic input=ortho_2001_t792_1m \
 output=ortho_smoothed_qa threshold=15 lambda=0.4 steps=20

# Smooth with exponential diffusivity function. Even better edge delineation
# but at the same time increase of noise in really noisy areas.
r.smooth.edgepreserve function=exponential input=ortho_2001_t792_1m \
 output=ortho_smoothed_ex threshold=15 lambda=0.4 steps=20

# Smooth with aggressive Tukey's diffusivity function. Better preservation of
# minor details e.g. as road markings but little smoothing in areas with
# fine, well expressed texture.
r.smooth.edgepreserve function=tukey input=ortho_2001_t792_1m \
 output=ortho_smoothed_ta threshold=15 lambda=0.4 steps=20

# Smooth with preserving Tukey's diffusivity function. Only low noise areas
# have been smoothed.
r.smooth.edgepreserve function=tukey input=ortho_2001_t792_1m \
 output=ortho_smoothed_tp threshold=15 lambda=0.4 steps=20 -p
```

## SEE ALSO

- Smooth with statistics: *[r.neighbours](r.neighbours)*
- The Mumford-Shah variational model for image segmentation (an add-on):
*[r.smooth.seg](https://grass.osgeo.org/grass-stable/manuals/addons/r.smooth.seg.html)*

## REFERENCES

- Perona P. and Malik J. 1990. Scale-space and edge detection using anisotropic
diffusion. *IEEE transactions on pattern analysis and machine intelligence*,
12(7).
- Black M.J., Sapiro G., Marimont D.H. and Heeger D. 1998. Robust anisotropic
diffusion. *IEEE transactions on image processing*, 7(3).

## AUTHOR

Maris Nartiss, University of Latvia.
