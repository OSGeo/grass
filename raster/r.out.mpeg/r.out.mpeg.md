## DESCRIPTION

*r.out.mpeg* is a tool for combining a series of GRASS raster maps into
a single MPEG-1 ([Motion Pictures Experts
Group](https://en.wikipedia.org/wiki/Moving_Picture_Experts_Group))
format file. MPEG-1 is a "lossy" video compression format, so the
quality of each resulting frame of the animation will be much diminished
from the original raster image. The resulting output file may then be
viewed using your favorite mpeg-format viewing program. MPEG-2 and
MPEG-4 provide much better quality animations.

The user may define up to four "views", or sub-windows, to animate
simultaneously. e.g., View 1 could be rainfall, View 2 flooded areas,
View 3 damage to bridges or levees, View 4 other economic damage, all
animated as a time series. A black border 2 pixels wide is drawn around
each view. There is an arbitrary limit of 400 files per view (400
animation frames). Temporary files are created in the conversion
process, so lack of adequate tmp space could also limit the number of
frames you are able to convert.

The environment variable GMPEG_SIZE is checked for a value to use as the
dimension, in pixels, of the longest dimension of the animation image.
If GMPEG_SIZE is not set, the animation size defaults to the rows &
columns in the current GRASS region, scaling if necessary to a default
minimum size of 200 and maximum of 500. These size defaults are
overridden when using the **-c** flag (see below). The resolution of the
current GRASS region is maintained, independent of image size. Playback
programs have to decode the compressed data "on-the-fly", therefore
smaller dimensioned animations will provide higher frame rates and
smoother animations.

UNIX - style wild cards may be used with the command line version in
place of a raster map name, but wild cards must be quoted.

A quality value of **quality=1** will yield higher quality images, but
with less compression (larger MPEG file size). Compression ratios will
vary depending on the number of frames in the animation, but an MPEG
produced using **quality=5** will usually be about 60% the size of the
MPEG produced using **quality=1**.

## EXAMPLES

```sh
r.out.mpeg view1="rain[1-9]","rain1[0-2]" view2="temp*"
```

If the number of files differs for each view, the view with the fewest
files will determine the number of frames in the animation.

With **-c** flag the module converts "on the fly", uses less disk space
by using *[r.out.ppm](r.out.ppm.md)* with stdout option to convert
frames as needed instead of converting all frames to ppm before
encoding. Only use when encoding a single view. Use of this option also
overrides any size defaults, using the **CURRENTLY DEFINED GRASS REGION
for the output size**. So be careful to set region to a reasonable size
prior to encoding.

## KNOWN ISSUES

MPEG images must be 16-pixel aligned for successful compression, so if
the rows & columns of the calculated image size (scaled, with borders
added) are not evenly divisible by 16, a few rows/columns will be cut
off the bottom & right sides of the image. The MPEG format is optimized
to recognize image MOTION, so abrupt changes from one frame to another
will cause a "noisy" encoding.

## NOTES

This program requires the program *mpeg_encode* (aka *ppmtompeg*):

MPEG-1 Video Software Encoder  
(Version 1.3; March 14, 1994)

Lawrence A. Rowe, Kevin Gong, Ketan Patel, and Dan Wallach Computer
Science Division-EECS, Univ. of Calif. at Berkeley

Available from Berkeley:
[http://biowiki.org/BerkeleyMpegEncoder](https://web.archive.org/web/20091223063452/http://biowiki.org/BerkeleyMpegEncoder)
(Wayback machine)  
or as part of the netpbm package (*ppmtompeg*):
<https://netpbm.sourceforge.net>

Use of the **-c** flag requires the *[r.out.ppm](r.out.ppm.md)* GRASS
module with the **stdout** option.

## SEE ALSO

*[r.out.ppm](r.out.ppm.md)*  

## AUTHOR

Bill Brown, U.S. Army Construction Engineering Research Laboratories
