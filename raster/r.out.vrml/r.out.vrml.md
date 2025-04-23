## DESCRIPTION

This module exports a GRASS raster map to the Virtual Reality Modeling
Language (VRML) format for 3D visualization.

This version only outputs raster maps in VRML 1.0 format. The newer VRML
2.0 format will be more efficient for geographic applications, as it
introduces an "ElevationGrid" node so that only the elevation points
will have to be written instead of the whole geometry. The vast majority
of VRML viewers currently only support VRML 1.0. If the extension "wrl"
(world) is not present in the he *output* parameter, it will be added.

## WARNING

VRML is not well suited for large geometries which can result from even
a small geographic region. Most viewers seem to bog down with more than
12,000 polygons, depending on your hardware & specific viewer. Each grid
cell results in two polygons, so a reasonable size region would be
something less than about 75x75. For improved performance and smaller
file size, leave off a color map. Since VRML is ascii text, gzip works
very well to significantly compress file size.

## NOTES

This is a preliminary release of "*r.out.vrml*". For further information
about VRML and available viewers for various platforms, see:

[VRML Virtual Reality Modeling
Language](https://www.w3.org/MarkUp/VRML/)

## BUGS

Currently the region is transformed to a unit size, so real geographic
location is lost. Side effects when working in a lat-lon project are
that besides general distortion due to projection, a very small
exaggeration factor (on order of .001) must be used to compensate for
vertical units expected to be the same as map units.

## TODO

Update to the more modern [GeoVRML format](http://www.geovrml.com/eng/),
or probably better the next generation [X3D
format](https://www.web3d.org/). See also the [Xj3D
project](https://gitlab.nps.edu/Savage/xj3d).

Future plans for this module are to allow draping of sites objects and
vector maps and using the new sites format available in floating point
GRASS to embed WWW links into site objects. It will also be upgraded to
support VRML 2.0 and will allow entering multiple preset "views" using
the existing GRASS 3d_view file format.

Other possible additions:

- Allow animation of elevation, color, or sites based on user
  interaction.
- Degradation of the raster to produce TINs for improved performance.

## AUTHOR

Bill Brown, US Army Construction Engineering Research Laboratory
