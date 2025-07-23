## DESCRIPTION

*i.pca* is an image processing program based on the algorithm provided
by Vali (1990), that processes n (n \>= 2) input raster map layers and
produces n output raster map layers containing the principal components
of the input data in decreasing order of variance ("contrast"). The
output raster map layers are assigned names with .1, .2, ... .n
suffixes. The numbers used as suffix correspond to percent importance
with .1 being the scores of the principal component with the highest
importance.

The current geographic region definition and raster mask settings are
respected when reading the input raster map layers. When the rescale
option is used, the output files are rescaled to fit the min,max range.

The order of the input bands does not matter for the output maps (PC
scores), but does matter for the vectors (loadings), since each loading
refers to a specific input band.

If the output is not rescaled (*rescale=0,0*, the output raster maps
will be of type DCELL, otherwise the output raster maps will be of type
CELL.

By default, the values of the input raster maps are centered for each
map separately with *x - mean*. With *-n*, the input raster maps are
normalized for each map separately with *(x - mean) / stddev*.
Normalizing is highly recommended when the input raster maps have
different units, e.g. represent different environmental parameters.

The *-f* flag, together with the *percent* option, can be used to remove
noise from input bands. Input bands will be recalculated from a subset
of the principal components (inverse PCA). The subset is selected by
using only the most important (highest eigenvalue) principal components
which explain together *percent* percent variance observed in the input
bands.

## NOTES

Richards (1986) gives a good example of the application of principal
components analysis (PCA) to a time series of LANDSAT images of a burned
region in Australia.

Eigenvalue and eigenvector information is stored in the output maps'
history files. View with *r.info*.

## EXAMPLE

PCA calculation using Landsat7 imagery in the North Carolina sample
dataset:

```sh
g.region raster=lsat7_2002_10 -p
i.pca in=lsat7_2002_10,lsat7_2002_20,lsat7_2002_30,lsat7_2002_40,lsat7_2002_50,lsat7_2002_70 \
    out=lsat7_2002_pca

r.info -h lsat7_2002_pca.1
   Eigen values, (vectors), and [percent importance]:
   PC1   4334.35 ( 0.2824, 0.3342, 0.5092,-0.0087, 0.5264, 0.5217) [83.04%]
   PC2    588.31 ( 0.2541, 0.1885, 0.2923,-0.7428,-0.5110,-0.0403) [11.27%]
   PC3    239.22 ( 0.3801, 0.3819, 0.2681, 0.6238,-0.4000,-0.2980) [ 4.58%]
   PC4     32.85 ( 0.1752,-0.0191,-0.4053, 0.1593,-0.4435, 0.7632) [ 0.63%]
   PC5     20.73 (-0.6170,-0.2514, 0.6059, 0.1734,-0.3235, 0.2330) [ 0.40%]
   PC6      4.08 (-0.5475, 0.8021,-0.2282,-0.0607,-0.0208, 0.0252) [ 0.08%]

d.mon wx0
d.rast lsat7_2002_pca.1
# ...
d.rast lsat7_2002_pca.6
```

In this example, the first two PCAs (PCA1 and PCA2) already explain
94.31% of the variance in the six input channels.

![PCA result](i_pca_result.png)  
Resulting PCA maps calculated from the Landsat7 imagery (NC, USA)

## SEE ALSO

Richards, John A., **Remote Sensing Digital Image Analysis**,
Springer-Verlag, 1986.

Vali, Ali R., Personal communication, Space Research Center, University
of Texas, Austin, 1990.

*[i.cca](i.cca.md), [g.gui.iclass](g.gui.iclass.md), [i.fft](i.fft.md),
[i.ifft](i.ifft.md),
[m.eigensystem](https://grass.osgeo.org/grass-stable/manuals/addons/m.eigensystem.html),
[r.covar](r.covar.md), [r.mapcalc](r.mapcalc.md)*

*[Principal Components Analysis
article](https://grasswiki.osgeo.org/wiki/Principal_Components_Analysis)
(GRASS Wiki)*

## AUTHORS

David Satnik, GIS Laboratory

Major modifications for GRASS 4.1 were made by  
Olga Waupotitsch and Michael Shapiro, U.S.Army Construction Engineering
Research Laboratory

Rewritten for GRASS 6.x and major modifications by  
Brad Douglas
