## DESCRIPTION

*r.in.mat* will import a GRASS raster map from a Version 4 MAT-File
which was created with Matlab or Octave. Attributes such as map title
and bounds will also be imported if they exist.  
  
Specifically, the following array variables will be read:  

- **map_data**
- **map_name**
- **map_title**
- **map_northern_edge**
- **map_southern_edge**
- **map_eastern_edge**
- **map_western_edge**

Any other variables in the MAT-file will be simply skipped over.  
  
The '**map_name**' variable is optional, if it exists, and is valid, the
new map will be thus named. If it doesn't exist or a name is specified
with the **output=** option, the raster map's name will be set to
"`MatFile`" or the name specified respectively. (maximum 64 characters;
normal GRASS naming rules apply)  
  
The '**map_title**' variable is optional, the map's title is set if it
exists.  
  
The '**map_northern_edge**' and like variables are mandatory unless the
user is importing to a "XY" non-georeferenced project (e.g. imagery
data). Latitude and longitude values should be in decimal form.

## NOTES

*r.in.mat* imports a Version 4 MAT-File. These files can be successfully
created with more modern versions of Matlab and Octave (see "EXAMPLES"
below).  
  
Everything should be Endian safe, so the file to be imported can be
simply copied between different system architectures without binary
translation (caveat: see "TODO" below).  
  
As there is no IEEE value for `NaN` in integer arrays, GRASS's null
value may be used to represent it within these maps. Usually Matlab will
save any integer based matrix with `NaN` values as a double-precision
floating point array, so this usually isn't an issue. To save space,
once the map is loaded into GRASS you can convert it back to an integer
map with the following command:

```sh
r.mapcalc "int_map = int(MATFile_map)"
```

`NaN` values in either floating point or double-precision floating point
matrices should translate into null values as expected.  
  
*r.in.mat* must load the entire map array into memory before writing,
therefore it might have problems with *huge* arrays. (a 3000x4000 DCELL
map uses about 100mb RAM)  
  
GRASS defines its map bounds at the outer-edge of the bounding cells,
not at the coordinates of their centroids. Thus, the following Matlab
commands may be used to determine and check the map's resolution
information will be correct:

```sh
    [rows cols] = size(map_data)
    x_range = map_eastern_edge - map_western_edge
    y_range = map_northern_edge - map_southern_edge
    ns_res = y_range/rows
    ew_res = x_range/cols
```

Remember Matlab arrays are referenced as `(row,column)`, i.e. `(y,x)`.  
  
In addition, *r.in.mat* and *r.out.mat* make for a nice binary container
format for transferring georeferenced maps around, even if you don't use
Matlab or Octave.

## EXAMPLES

In Matlab, save with:

```sh
save filename.mat map_* -v4
```

In Octave, save with:

```sh
save -mat4-binary filename.mat map_*
```

## TODO

Robust support for mixed-Endian importation. *(This is a work in
progress, please help by reporting any failures to the [GRASS bug
tracking system](https://github.com/OSGeo/grass/issues)*;  
Add support for importing map history, category information, color map,
etc. if they exist.  
Option to import a version 5 MAT-File, with map and support information
stored in a single structured array.

## KNOWN ISSUES

If you encounter any problems, please contact the GRASS Development
Team.

## SEE ALSO

*[r.out.mat](r.out.mat.md), [r.in.ascii](r.in.ascii.md),
[r.in.bin](r.in.bin.md), [r.mapcalc](r.mapcalc.md),
[r.null](r.null.md).*

*The [Octave](http://www.octave.org) project*

## AUTHOR

Hamish Bowman  
*Department of Marine Science  
University of Otago  
New Zealand*
