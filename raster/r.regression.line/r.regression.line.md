## DESCRIPTION

*r.regression.line* calculates a linear regression from two raster maps,
according to the formula

```sh
y = a + b*x
```

where

```sh
x
y
```

represent the input raster maps.

Optionally, it saves regression coefficients as a ASCII file. The result
includes the following coefficients: offset/intercept (a) and gain/slope
(b), correlation coefficient (R), number of elements (N), means (medX,
medY), standard deviations (sdX, sdY), and the F test for testing the
significance of the regression model as a whole (F).

## NOTES

The results for offset/intercept (a) and gain/slope (b) are identical to
that obtained from R-stats's lm() function.

## EXAMPLE

Comparison of two DEMs (SRTM and NED, both at 30m resolution), provided
in the North Carolina sample dataset:

```sh
g.region raster=elev_srtm_30m -p
r.regression.line mapx=elev_ned_30m mapy=elev_srtm_30m
 y = a + b*x
   a (Offset): -1.659279
   b (Gain): 1.043968
   R (sumXY - sumX*sumY/N): 0.894038
   N (Number of elements): 225000
   F (F-test significance): 896093.366283
   meanX (Mean of map1): 110.307571
   sdX (Standard deviation of map1): 20.311998
   meanY (Mean of map2): 113.498292
   sdY (Standard deviation of map2): 23.718307
```

Using the script style flag AND *eval* to make results available in the
shell:

```sh
g.region raster=elev_srtm_30m -p
eval `r.regression.line -g mapx=elev_ned_30m mapy=elev_srtm_30m`

# print result stored in respective variables
echo $a
-1.659279

echo $b
1.043968

echo $R
0.894038
```

## SEE ALSO

*[d.correlate](d.correlate.md),
[r.regression.multi](r.regression.multi.md), [r.stats](r.stats.md)*

## AUTHORS

Dr. Agustin Lobo - alobo at ija.csic.es  
Updated to GRASS 5.7 Michael Barton, Arizona State University  
Script style output Markus Neteler  
Conversion to C module Markus Metz
