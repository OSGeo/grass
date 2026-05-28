## DESCRIPTION

*m.cogo* converts data points between bearing and distance and X,Y
coordinates. Only simple bearing/distance or coordinate pairs are
handled. It assumes a cartesian coordinate system.

Input can be entered via standard input (default) or from the file
**input**=*name*. Specifying the input as "-" also specifies standard
input, and is useful for using the program in a pipeline. Output will be
to standard output unless a file name other than "-" is specified. The
input file must closely adhere to the following format, where up to a 10
character label is allowed but not required (see **-l** flag).

**Example COGO input:**

```sh
   P23 N 23:14:12 W 340
   P24 S 04:18:56 E 230
   ...
```

The first column may contain a label and you must use the **-l** flag so
the program knows. This is followed by a space, and then either the
character 'N' or 'S' to indicate whether the bearing is relative to the
north or south directions. After another space, the angle begins in
degrees, minutes, and seconds in "DDD:MM:SS.SSSS" format. Generally, the
angle can be of the form *digits + separator + digits + separator +
digits \[+ '.' + digits\]*. A space follows the angle, and is then
followed by either the 'E' or 'W' characters. A space separates the
bearing from the distance (which should be in appropriate linear units).

**Output of the above input:**

```sh
   -134.140211 312.420236 P23
   -116.832837 83.072345 P24
   ...
```

Unless specified with the **coord** option, calculations begin from
(0,0).

## NOTES

For those unfamiliar with the notation for bearings: Picture yourself in
the center of a circle. The first hemispere notation tell you whether
you should face north or south. Then you read the angle and either turn
that many degrees to the east or west, depending on the second
hemisphere notation. Finally, you move \<distance\> units in that
direction to get to the next station.

*m.cogo* can be run either non-interactively or interactively. The
program will be run non-interactively if the user specifies any
parameter or flag. Use "m.cogo -", to run the program in a pipeline.
Without any flags or parameters, *m.cogo* will prompt for each value
using the familiar GRASS parser interface.

This program is very simplistic, and will not handle deviations from the
input format explained above. Currently, the program doesn't do anything
particularly useful with the output. However, it is envisioned that this
program will be extended to provide the capability to generate vector
and/or sites layers.

Lines may be closed by using the **-c** flag or snapped with *v.clean*,
lines may be converted to boundaries with *v.type*, and closed
boundaries may be converted to areas with *v.centroids*.

## EXAMPLES

```sh
   m.cogo -l in=cogo.dat
```

Where the `cogo.dat` input file looks like:

```sh
# Sample COGO input file -- This defines an area.
# <label> <bearing> <distance>
P001 S 88:44:56 W 6.7195
P002 N 33:34:15 W 2.25
P003 N 23:23:50 W 31.4024
P004 N 05:04:45 W 25.6981
P005 N 18:07:25 E 22.2439
P006 N 27:49:50 E 75.7317
P007 N 22:56:50 E 87.4482
P008 N 37:45:15 E 37.7835
P009 N 46:04:30 E 11.5854
P010 N 90:00:00 E 8.8201
P011 N 90:00:00 E 164.1128
P012 S 48:41:12 E 10.1311
P013 S 00:25:50 W 255.7652
P014 N 88:03:13 W 98.8567
P015 S 88:44:56 W 146.2713
P016 S 88:44:56 W 18.7164
```

Round trip:

```sh
   m.cogo -l input=cogo.dat | m.cogo -rl in="-"
```

Import as a vector points map:

```sh
   m.cogo -l input=cogo.dat | v.in.ascii output=cogo_points x=1 y=2 separator=space
```

Shell script to import as a vector line map:

```sh
   m.cogo -l input=cogo.dat | tac | awk '
       BEGIN { FS=" " ; R=0 }
       $1~/\d*\.\d*/ { printf(" %.8f %.8f\n", $1, $2) ; ++R }
       END { printf("L %d\n", R) }' | tac | \
       v.in.ascii -n format=standard out=cogo_line
```

Convert that lines map into an area:

```sh
   # Add the -c flag to the above example to close the loop:
   m.cogo -l -c input=cogo.dat | ...
       ...
   v.type input=cogo_line output=cogo_boundary from_type=line to_type=boundary
   v.centroids input=cogo_boundary output=cogo_area
```

If necessary, snap the boundary closed with the *v.clean* module. Use
`tool=snap` and `thresh=0.0001`, or some small value.

## SEE ALSO

*[v.centroids](v.centroids.md), [v.clean](v.clean.md), [wxGUI vector
digitizer](wxGUI.vdigit.md), [v.in.ascii](v.in.ascii.md),
[v.type](v.type.md)*

## AUTHOR

Eric G. Miller
