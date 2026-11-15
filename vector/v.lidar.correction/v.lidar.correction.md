## DESCRIPTION

*v.lidar.correction* is the last of three steps to filter LiDAR data.
The filter aims to recognize and extract attached and detached object
(such as buildings, bridges, power lines, trees, etc.) in order to
create a Digital Terrain Model.  
  
The module, which could be iterated several times, makes a comparison
between the LiDAR observations and a bilinear spline interpolation with
a Tychonov regularization parameter performed on the TERRAIN SINGLE
PULSE points only. The gradient is minimized by the regularization
parameter. Analysis of the residuals between the observations and the
interpolated values results in four cases (the next classification is
referred to that of the v.lidar.growing output vector):  
  
**a)** Points classified as TERRAIN differing more than a threshold
value are interpreted and reclassified as OBJECT, for both single and
double pulse points.  
  
**b)** Points classified as OBJECT and closed enough to the interpolated
surface are interpreted and reclassified as TERRAIN, for both single and
double pulse points.

The length (in mapping units) of each spline step is defined by
**ew_step** for the east-west direction and **ns_step** for the
north-south direction.

## NOTES

The input should be the output of *v.lidar.growing* module or the output
of this *v.lidar.correction* itself. That means, this module could be
applied more times (although, two are usually enough) for a better
filter solution. The outputs are a vector map with a final point
classification as as TERRAIN SINGLE PULSE, TERRAIN DOUBLE PULSE, OBJECT
SINGLE PULSE or OBJECT DOUBLE PULSE; and an vector map with only the
points classified as TERRAIN SINGLE PULSE or TERRAIN DOUBLE PULSE. The
final result of the whole procedure (*v.lidar.edgedetection*,
*v.lidar.growing*, *v.lidar.correction*) will be a point classification
in four categories:  
  
TERRAIN SINGLE PULSE (cat = 1, layer = 2)  
TERRAIN DOUBLE PULSE (cat = 2, layer = 2)  
OBJECT SINGLE PULSE (cat = 3, layer = 2)  
OBJECT DOUBLE PULSE (cat = 4, layer = 2)

## EXAMPLES

### Basic correction procedure

```sh
v.lidar.correction input=growing output=correction out_terrain=only_terrain
```

### Second correction procedure

```sh
v.lidar.correction input=correction output=correction_bis terrain=only_terrain_bis
```

## REFERENCES

Antolin, R. et al., 2006. Digital terrain models determination by LiDAR
technology: Po basin experimentation. Bolletino di Geodesia e Scienze
Affini, anno LXV, n. 2, pp. 69-89.  
  
Brovelli M. A., Cannata M., Longoni U.M., 2004. LIDAR Data Filtering and
DTM Interpolation Within GRASS, Transactions in GIS, April 2004, vol. 8,
iss. 2, pp. 155-174(20), Blackwell Publishing Ltd.  
  
Brovelli M. A., Cannata M., 2004. Digital Terrain model reconstruction
in urban areas from airborne laser scanning data: the method and an
example for Pavia (Northern Italy). Computers and Geosciences 30 (2004)
pp.325-331  
  
Brovelli M. A. and Longoni U.M., 2003. Software per il filtraggio di
dati LIDAR, Rivista dell'Agenzia del Territorio, n. 3-2003, pp. 11-22
(ISSN 1593-2192).  
  
Brovelli M. A., Cannata M. and Longoni U.M., 2002. DTM LIDAR in area
urbana, Bollettino SIFET N.2, pp. 7-26.  
  
Performances of the filter can be seen in the [ISPRS WG III/3 Comparison
of Filters](https://www.itc.nl/isprs/wgIII-3/filtertest/) report by
Sithole, G. and Vosselman, G., 2003.

## SEE ALSO

*[v.lidar.edgedetection](v.lidar.edgedetection.md),
[v.lidar.growing](v.lidar.growing.md),
[v.surf.bspline](v.surf.bspline.md), [v.surf.rst](v.surf.rst.md),
[v.in.pdal](v.in.pdal.md), [v.in.ascii](v.in.ascii.md)*

## AUTHORS

Original version of program in GRASS 5.4:  
Maria Antonia Brovelli, Massimiliano Cannata, Ulisse Longoni and Mirko
Reguzzoni  
  
Update for GRASS 6.X:  
Roberto Antolin and Gonzalo Moreno
