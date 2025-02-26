## DESCRIPTION

*r.to.rast3* converts 2D raster map(s) into one raster3D map. It is
important to properly set the 3D region settings, especially number or
layers and depth of layers. If the 2D and 3D region settings are
different, the 2D resolution will be adjusted to the 3D resolution.

![How r.to.rast3 works](r.to.rast3.png)  
*How r.to.rast3 works*

## NOTES

Every 2D raster map is copied as one slice to the raster3D map. Slices
are counted from bottom to the top, so the bottom slice has to be number
1.  
  
If less number of 2D raster maps are provided than defined depths, the
last given 2D map is used to fill up the remaining raster3D slices to
the top.

## EXAMPLES

### EXAMPLE 1

This example shows how to convert 6 maps into one 3D map with 6
layers.  

```sh
# Mapset data in dataset slovakia3d
g.region raster=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 -p
g.region b=0 t=600 tbres=100 res3=100 -p3
r.to.rast3 input=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 output=new_3dmap
```

### EXAMPLE 2

This example shows how to convert 3 maps into one 3D map with 6
layers.  

```sh
# Mapset data in dataset slovakia3d
g.region b=0 t=600 tbres=100 res3=100 -p3
r.to.rast3 input=prec_1,prec_2,prec_3 output=new_3dmap
```

## SEE ALSO

*[g.region](g.region.md), [r3.to.rast](r3.to.rast.md),
[r.to.rast3elev](r.to.rast3elev.md)*

## AUTHOR

Soeren Gebbert
