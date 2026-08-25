## DESCRIPTION

*r.out.mat* will export a GRASS raster map to a MAT-File which can be
loaded into Matlab or Octave for plotting or further analysis.
Attributes such as map title and bounds will also be exported into
additional array variables.  
  
Specifically, the following array variables are created:  

- **map_data**
- **map_name**
- **map_title** (if it exists)
- **map_northern_edge**
- **map_southern_edge**
- **map_eastern_edge**
- **map_western_edge**

In addition, *r.out.mat* makes for a nice binary container format for
transferring georeferenced maps around, even if you don't use Matlab or
Octave.

## NOTES

*r.out.mat* exports a Version 4 MAT-File. These files should
successfully load into more modern versions of Matlab and Octave without
any problems.  
  
Everything should be Endian safe, so the resultant file can be simply
copied between different system architectures without binary
translation.  
  
As there is no IEEE value for `NaN` for integer maps, GRASS's null value
is used to represent it within these maps. You'll have to do something
like this to clean them once the map is loaded into Matlab:

```sh
    map_data(find(map_data < -1e9)) = NaN;
```

Null values in maps containing either floating point or double-precision
floating point data should translate into `NaN` values as expected.  
  
*r.out.mat* must load the entire map into memory before writing,
therefore it might have problems with *huge* maps. (a 3000x4000 DCELL
map uses about 100mb RAM)  
  
GRASS defines its map bounds at the outer-edge of the bounding cells,
not at the coordinates of their centroids. Thus, the following Matlab
commands may be used to determine the map's resolution information:

```sh
    [rows cols] = size(map_data)
    x_range = map_eastern_edge - map_western_edge
    y_range = map_northern_edge - map_southern_edge
    ns_res = y_range/rows
    ew_res = x_range/cols
```

## EXAMPLE

In Matlab, plot with either:

```sh
imagesc(map_data), axis equal, axis tight, colorbar
```

or

```sh
contourf(map_data, 24), axis ij, axis equal, axis tight, colorbar
```

## TODO

Add support for exporting map history, category information, color map,
etc.  
Option to export as a version 5 MAT-File, with map and support
information stored in a single structured array.

## SEE ALSO

*[r.in.mat](r.in.mat.md)  
[r.out.ascii](r.out.ascii.md), [r.out.bin](r.out.bin.md)  
[r.null](r.null.md)  
The [Octave](http://www.octave.org) project*

## AUTHOR

Hamish Bowman  
*Department of Marine Science  
University of Otago  
New Zealand*  
