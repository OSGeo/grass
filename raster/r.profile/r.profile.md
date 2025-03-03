## DESCRIPTION

This program outputs two or four column (with **-g**) data to stdout or
an ASCII file. The default two column output consists of cumulative
profile length and raster value. The optional four column output
consists of easting, northing, cumulative profile length, and raster
value. Profile end or "turning" points can be set manually with the
**coordinates** argument. The profile resolution, or distance between
profile points, is obtained from the current region resolution, or can
be manually set with the **resolution** argument.

The **coordinates** parameter can be set to comma separated geographic
coordinates for profile line endpoints. Alternatively the coordinate
pairs can be piped from the text file specified by **file** option, or
if set to "-", from `stdin`. In these cases the coordinate pairs should
be given one comma separated pair per line.

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
coordinates (*-g*) is compatible with *[v.in.ascii](v.in.ascii.md)* and
can be piped directly into this program.

```sh
r.profile -g input=elevation coordinates=... | v.in.ascii output=elevation_profile separator=space
```

The 2 column output is compatible with most plotting programs.

The optional RGB output provides the associated GRASS colour value for
each profile point.

Option **units** enables to set units of the profile length output. If
the units are not specified, current coordinate reference system's units
will be used. In case of geographic CRS (latitude/longitude), meters are
used as default unit. Finally, the output from *r.info* can be output in
JSON by passing the **format=json** option.

## NOTES

The profile resolution is measured exactly from the supplied end or
"turning" point along the profile. The end of a profile segment will be
an exact multiple of the profile resolution and will therefore not
always match the end point coordinates entered for the segmanet.

To extract the numbers in scripts, following parameters can be used:

```sh
r.profile input=dgm12.5 coordinates=3570631,5763556 2>/dev/null
```

This filters out everything except the numbers.

## EXAMPLES

### Extraction of values along profile defined by coordinates (variant 1)

Extract a profile with coordinates (waypoints) provided on the command
line (North Carolina data set):

```sh
g.region raster=elevation -p
r.profile -g input=elevation output=profile_points.csv \
          coordinates=641712,226095,641546,224138,641546,222048,641049,221186
```

This will extract a profile along the track defined by the three
coordinate pairs. The output file "profile_points.csv" contains
east,north,distance,value (here: elevation).

### Extraction of values along profile defined by coordinates (variant 2)

Coordinate pairs can also be "piped" into *r.profile* (variant 2a):

```sh
r.profile elevation resolution=1000 file=- << EOF
641712,226095
641546,224138
641546,222048
641049,221186
EOF
```

Coordinate pairs can also be "piped" into *r.profile* (variant 2b):

```sh
echo "641712,226095
641546,224138
641546,222048
641049,221186" > coors.txt
cat coors.txt | r.profile elevation resolution=1000 file=-
```

The output is printed into the terminal (unless the *output* parameter
is used) and looks as follows:

```sh
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
```

### JSON Output

```sh
r.profile -g input=elevation coordinates=641712,226095,641546,224138,641546,222048,641049,221186 -c format=json resolution=1000
```

The output looks as follows:

```json
[
    {
        "easting": 641712,
        "northing": 226095,
        "distance": 0,
        "elevation": 84.661506652832031,
        "red": 113,
        "green": 255,
        "blue": 0
    },
    {
        "easting": 641627.47980925441,
        "northing": 225098.57823319823,
        "distance": 1000.0000000000125,
        "elevation": 98.179061889648438,
        "red": 255,
        "green": 241,
        "blue": 0
    },
    {
        "easting": 641546,
        "northing": 224138,
        "distance": 1964.0277492948007,
        "elevation": 83.638137817382812,
        "red": 100,
        "green": 255,
        "blue": 0
    },
    {
        "easting": 641546,
        "northing": 223138,
        "distance": 2964.0277492948007,
        "elevation": 89.141029357910156,
        "red": 169,
        "green": 255,
        "blue": 0
    },
    {
        "easting": 641546,
        "northing": 222138,
        "distance": 3964.0277492948007,
        "elevation": 78.497756958007812,
        "red": 35,
        "green": 255,
        "blue": 0
    },
    {
        "easting": 641546,
        "northing": 222048,
        "distance": 4054.0277492948007,
        "elevation": 73.988029479980469,
        "red": 0,
        "green": 249,
        "blue": 17
    }
]
```

### Using JSON output with Python for plotting data

The JSON output makes for ease of integration with popular python data
science libraries. For instance, here is an example of creating a
scatterplot of distance vs elevation with color coding.

```python
import grass.script as gs
import pandas as pd
import matplotlib.pyplot as plt

# Run r.profile command
elevation = gs.read_command(
    "r.profile",
    input="elevation",
    coordinates="641712,226095,641546,224138,641546,222048,641049,221186",
    format="json",
    flags="gc"
)

# Load the JSON data into a dataframe
df = pd.read_json(elevation)

# Convert the RGB color values to hex format for matplotlib
df["color"] = df.apply(lambda x: "#{:02x}{:02x}{:02x}".format(int(x["red"]), int(x["green"]), int(x["blue"])), axis=1)

# Create the scatter plot
plt.figure(figsize=(10, 6))
plt.scatter(df['distance'], df['elevation'], c=df['color'], marker='o')
plt.title('Profile of Distance vs. Elevation with Color Coding')
plt.xlabel('Distance (meters)')
plt.ylabel('Elevation')
plt.grid(True)
plt.show()
```

## SEE ALSO

*[v.in.ascii](v.in.ascii.md), [r.what](r.what.md),
[r.transect](r.transect.md), [wxGUI profile tool](wxGUI.md)*

## AUTHOR

[Bob Covill](mailto:bcovill@tekmap.ns.ca)
