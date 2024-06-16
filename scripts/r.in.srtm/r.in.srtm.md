## DESCRIPTION

*r.in.srtm* imports SRTM HGT files into GRASS. SRTM Version 1 and
improved Version 2 data sets can be downloaded from NASA at this site:\
<http://dds.cr.usgs.gov/srtm/>

Gap-filled SRTM Version 3 data can be downloaded from USGS at this
site:\
<https://e4ftl01.cr.usgs.gov/MEASURES/SRTMGL3.003/2000.02.11/>

## NOTES

SRTM tiles are of 1 degree by 1 degree size. The SRTM filename contains
the coordinates which refer to the **center** of the lower left pixel
(e.g., N51E010: lower left cell center at 10E, 51N). To identify a tile
name, a grid can be easily visualized by *[d.grid](d.grid.html)*:

::: code
    d.grid size=1
:::

To import TOPEX/SRTM30 PLUS data, use *[r.in.bin](r.in.bin.html)*.

## REFERENCES

M. Neteler, 2005. [SRTM and VMAP0 data in OGR and
GRASS.](https://grass.osgeo.org/newsletter/GRASSNews_vol3.pdf) *[GRASS
Newsletter](https://grass.osgeo.org/newsletter/)*, Vol.3, pp. 2-6, June
2005. ISSN 1614-8746.

## SEE ALSO

*[r.in.bin](r.in.bin.html),
[r.in.srtm.region](https://grass.osgeo.org/grass8/manuals/addons/r.in.srtm.region.html)
(Addon),
[r.in.nasadem](https://grass.osgeo.org/grass8/manuals/addons/r.in.nasadem.html)
(Addon)*

The [Shuttle Radar Topography Mission](http://www2.jpl.nasa.gov/srtm/)
homepage at NASA\'s JPL.\
The [SRTM Web Forum](http://pub7.bravenet.com/forum/537683448/)

## AUTHORS

Markus Neteler\
Improved by W. Kyngesburye and H. Bowman\
Update for SRTM V3 by Markus Metz
