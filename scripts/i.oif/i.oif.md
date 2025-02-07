## DESCRIPTION

*i.oif* calculates the Optimum Index Factor for multi-spectral satellite
imagery.

The Optimum Index Factor (OIF) determines the three-band combination
that maximizes the variability (information) in a multi-spectral scene.
The index is a ratio of the total variance (standard deviation) within
and the correlation between all possible band combinations. The bands
that comprise the highest scoring combination from *i.oif* are used as
the three color channels required for *d.rgb* or *r.composite*.

The analysis is saved to a file in the current directory called
"i.oif.result".

## NOTES

Landsat 1-7 TM: Colour Composites in BGR order as important Landsat TM
band combinations (example: 234 in BGR order means: B=2, G=3, R=4):

- 123: near natural ("true") colour; however, because of correlation of
  the 3 bands in visible spectrum, this combination contains not much
  more info than is contained in single band.
- 234: sensitive to green vegetation (portrayed as red), coniferous as
  distinctly darker red than deciduous forests. Roads and water bodies
  are clear.
- 243: green vegetation is green but coniferous forests aren't as clear
  as the 234 combination.
- 247: one of the best for info pertaining to forestry. Good for
  operation scale mapping of recent harvest areas and road construction.
- 345: contains one band from each of the main reflective units (vis,
  nir, shortwave infra). Green vegetation is green and the shortwave
  band shows vegetational stress and mortality. Roads are less evident
  as band 3 is blue.
- 347: similar to 345 but depicts burned areas better.
- 354: appears more like a colour infrared photo.
- 374: similar to 354.
- 457: shows soil texture classes (clay, loam, sandy).

By default the module will calculate standard deviations for all bands
in parallel. To run serially use the **-s** flag. If the `WORKERS`
environment variable is set, the number of concurrent processes will be
limited to that number of jobs.

## EXAMPLE

North Carolina sample dataset:

```sh
g.region raster=lsat7_2002_10 -p
i.oif input=lsat7_2002_10,lsat7_2002_20,lsat7_2002_30,lsat7_2002_40,lsat7_2002_50,lsat7_2002_70
```

## REFERENCES

Jensen, 1996. Introductory digital image processing. Prentice Hall,
p.98. ISBN 0-13-205840-5

## SEE ALSO

*[d.rgb](d.rgb.md), [r.composite](r.composite.md),
[r.covar](r.covar.md), [r.univar](r.univar.md)*

## AUTHORS

Markus Neteler, ITC-Irst, Trento, Italy  
Updated to GRASS 5.7 by Michael Barton, Arizona State University
