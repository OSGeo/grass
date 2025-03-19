## DESCRIPTION

**r.surf.fractal** creates a fractal surface of a given fractal
dimension. It uses the spectral synthesis method. The module can create
intermediate layers showing the build up of different spectral
coefficients (see Saupe, pp.106-107 for an example of this).

This module generates naturally looking synthetical elevation models
(DEM).

## NOTE

This module requires the [FFTW library](https://fftw.org) for computing
Discrete Fourier Transforms.

## EXAMPLE

Generate surface using fractals in selected region, set color table and
display with shade.

```sh
g.region -p raster=elevation

r.surf.fractal output=fractals

r.colors map=fractals color=byr
r.relief input=fractals output=fractals_shade

d.mon wx0
d.shade shade=fractals_shade color=fractals b=50
```

![Artificial surface created with fractals](r_surf_fractal_simple.png)  
Artificial surface created with fractals  

Compare results when using different fractal dimensions:

```sh
# D=2.0005
g.region -dp
r.surf.fractal out=dem_d2_0005 dim=2.0005
r.info -r dem_d2_0005
r.mapcalc "dem_d2_0005_final = 1.0 * dem_d2_0005 + abs(min(dem_d2_0005))"
r.colors dem_d2_0005_final color=terrain
r.slope.aspect dem_d2_0005_final aspect=dem_d2_0005_final_as

# D=2.90
r.surf.fractal out=dem_d2_90 dim=2.90
r.info -r dem_d2_90
r.mapcalc "dem_d2_90_final = 1.0 * dem_d2_90 + abs(min(dem_d2_90))"
r.colors dem_d2_90_final color=terrain
r.slope.aspect dem_d2_90_final aspect=dem_d2_90_final_as
```

![Artificial DEMs created with fractals](r_surf_fractal.jpg)  
Artificial DEMs created with fractals:  
top: fractal dimension d=2.0005 (left: elevation map, right: aspect
map)  
top: fractal dimension d=2.90 (left: elevation map, right: aspect map)

## REFERENCES

Saupe, D. (1988) Algorithms for random fractals, in Barnsley M., Devaney
R., Mandelbrot B., Peitgen, H-O., Saupe D., and Voss R. (1988) The
Science of Fractal Images, Ch. 2, pp.71-136. London: Springer-Verlag.

## SEE ALSO

*[r.surf.contour](r.surf.contour.md), [r.surf.idw](r.surf.idw.md),
[r.surf.gauss](r.surf.gauss.md), [r.surf.random](r.surf.random.md),
[v.surf.idw](v.surf.idw.md), [v.surf.rst](v.surf.rst.md)*

## AUTHOR

Jo Wood, Midlands Regional Research Laboratory (ASSIST), University of
Leicester
