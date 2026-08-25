## DESCRIPTION

The *r.out.xyz* module exports a raster map as a list of x,y,z values
into an ASCII text file.

## NOTES

This module will by default not export x,y coordinates for raster cells
containing a NULL value. This includes cells masked by a raster mask.
Using the flag **-i** also these raster cells will be included in the
exported data.

This module, as all GRASS raster modules, will export cells based on the
current region settings. See the *g.region* module for details.

The *r.out.ascii* module should be used to export an array (of size row
x column) containing z values.

*r.out.xyz* can combine several input raster maps, which can be
convenient when it comes to e.g. produce ASCII point cloud files.

*r.out.xyz* is simply a front-end to "`r.stats -1g[n]`".

## EXAMPLES

In this example, a LiDAR elevation map in the North Carolina sample
dataset is exported to CSV format.

```sh
g.region raster=elev_lid792_1m -p
r.out.xyz input=elev_lid792_1m output=elev_lid792_1m.csv separator=","
```

In this example, elevation data from the North Carolina dataset are
exported along with R,G,B triplet of the related orthophoto into a
combined file (requires the import of the supplementary high-resolution
[color
orthophoto](https://grass.osgeo.org/sampledata/north_carolina/ortho2010_t792_subset_20cm.tif),
here called "ortho2010_t792"):

```sh
g.region raster=elev_lid792_1m res=1 -a -p
r.out.xyz input=elev_lid792_1m,ortho2010_t792.red,ortho2010_t792.green,ortho2010_t792.blue \
        separator=space output=pointcloud.asc

# validate: X Y Z R G B
head -n 3 pointcloud.asc
638300.5 220749.5 126.338218689 78 84 71
638301.5 220749.5 126.3381958008 93 101 86
638302.5 220749.5 126.3414840698 68 77 59
```

## TODO

Implement this script as a *r.out.ascii* option?

## SEE ALSO

*[g.region](g.region.md), [r.mask](r.mask.md)
[r.out.ascii](r.out.ascii.md), [r.stats](r.stats.md)*

## AUTHOR

M. Hamish Bowman  
*Dept. Marine Science  
Otago University, New Zealand*
