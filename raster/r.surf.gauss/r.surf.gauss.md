## DESCRIPTION

*r.surf.gauss* produces a raster map of Gaussian deviates whose mean and
standard deviation can be expressed by the user. It uses a Gaussian
random number generator. It is essentially the same as
*[r.surf.random](r.surf.random.html)*, but uses a Gaussian random number
generator instead.

## EXAMPLE

::: code
    g.region -p n=228500 s=215000 w=630000 e=645000 res=10
    r.surf.gauss out=gauss mean=0 sigma=10

    # check result
    r.univar gauss
:::

::: {align="center" style="margin: 10px"}
[![r.surf.gauss example (mean: 0; sigma:
10)](r_surf_gauss.jpg){width="600" height="295"
border="0"}](r_surf_gauss.jpg)\
*Figure: Random Gaussian surface example (mean: 0; sigma: 10)*
:::

With the histogram tool the cell values versus count can be shown.

::: {align="center" style="margin: 10px"}
[![r.surf.gauss example histogram (mean: 0; sigma:
10)](r_surf_gauss_hist.png){width="600" height="275"
border="0"}](r_surf_gauss_hist.png)\
*Figure: Histogram of random Gaussian surface example (mean: 0; sigma:
10)*
:::

## SEE ALSO

*[r.surf.contour](r.surf.contour.html),
[r.surf.fractal](r.surf.fractal.html), [r.surf.idw](r.surf.idw.html),
[r.surf.random](r.surf.random.html), [v.surf.rst](v.surf.rst.html)*

## AUTHOR

Jo Wood, [ASSIST\'s home](http://www.geog.le.ac.uk/assist/index.html)
