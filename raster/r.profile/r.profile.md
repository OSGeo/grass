## DESCRIPTION

This program outputs two or four column (with **-g**) data to stdout or
an ASCII file. The default two column output consists of cumulative
profile length and raster value. The optional four column output
consists of easting, northing, cumulative profile length, and raster
value. Profile end or \"turning\" points can be set manually with the
**coordinates** argument. The profile resolution, or distance between
profile points, is obtained from the current region resolution, or can
be manually set with the **resolution** argument.

The **coordinates** parameter can be set to comma separated geographic
coordinates for profile line endpoints. Alternatively the coordinate
pairs can be piped from the text file specified by **file** option, or
if set to \"-\", from `stdin`. In these cases the coordinate pairs
should be given one comma separated pair per line.

The **resolution** parameter sets the distance between each profile
point (resolution). The resolution must be provided in GRASS database
units (i.e. decimal degrees for Lat Long databases and meters for UTM).
By default *r.profile* uses the resolution of the current GRASS region.

The **null** parameter can optionally be set to change the character
string representing null values.

## OUTPUT FORMAT

The multi column output from *r.profile* is intended for easy use in
other programs. The output can be piped (\|) directly into other
programs or saved to a file for later use. Output with geographic
coordinates (*-g*) is compatible with *[v.in.ascii](v.in.ascii.html)*
and can be piped directly into this program.

::: code
    r.profile -g input=elevation coordinates=... | v.in.ascii output=elevation_profile separator=space
:::

The 2 column output is compatible with most plotting programs.

The optional RGB output provides the associated GRASS colour value for
each profile point.

Option **units** enables to set units of the profile length output. If
the units are not specified, current coordinate reference system\'s
units will be used. In case of geographic CRS (latitude/longitude),
meters are used as default unit.

## NOTES

The profile resolution is measured exactly from the supplied end or
\"turning\" point along the profile. The end of a profile segment will
be an exact multiple of the profile resolution and will therefore not
always match the end point coordinates entered for the segmanet.

To extract the numbers in scripts, following parameters can be used:

::: code
    r.profile input=dgm12.5 coordinates=3570631,5763556 2>/dev/null
:::

This filters out everything except the numbers.

## EXAMPLES

### Extraction of values along profile defined by coordinates (variant 1)

Extract a profile with coordinates (wayoints) provided on the command
line (North Carolina data set):

::: code
    g.region raster=elevation -p
    r.profile -g input=elevation output=profile_points.csv \
              coordinates=641712,226095,641546,224138,641546,222048,641049,221186
:::

This will extract a profile along the track defined by the three
coordinate pairs. The output file \"profile_points.csv\" contains
east,north,distance,value (here: elevation).

\

### Extraction of values along profile defined by coordinates (variant 2)

Coordinate pairs can also be \"piped\" into *r.profile* (variant 2a):

::: code
    r.profile elevation resolution=1000 file=- << EOF
    641712,226095
    641546,224138
    641546,222048
    641049,221186
    EOF
:::

Coordinate pairs can also be \"piped\" into *r.profile* (variant 2b):

::: code
    echo "641712,226095
    641546,224138
    641546,222048
    641049,221186" > coors.txt
    cat coors.txt | r.profile elevation resolution=1000 file=-
:::

The output is printed into the terminal (unless the *output* parameter
is used) and looks as follows:

::: code
    Using resolution: 1000 [meters]
    Output columns:
    Along track dist. [meters], Elevation
    Approx. transect length: 1964.027749 [meters]
     0.000000 84.661507
     1000.000000 98.179062
    Approx. transect length: 2090.000000 [meters]
     1964.027749 83.638138
     2964.027749 89.141029
     3964.027749 78.497757
    Approx. transect length: 995.014070 [meters]
     4054.027749 73.988029
:::

## SEE ALSO

*[v.in.ascii](v.in.ascii.html), [r.what](r.what.html),
[r.transect](r.transect.html), [wxGUI profile tool](wxGUI.html)*

## AUTHOR

[Bob Covill](mailto:bcovill@tekmap.ns.ca)
