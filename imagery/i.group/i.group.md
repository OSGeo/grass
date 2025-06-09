## DESCRIPTION

*i.group* allows the user to collect raster map layers in an imagery
group by assigning them to user-named subgroups or other groups. This
enables the user to run analyses on any combination of the raster map
layers in a group. The user creates the groups and subgroups and selects
the raster map layers that are to reside in them. Imagery analysis
programs like *[g.gui.gcp](g.gui.gcp.md)*, *[i.rectify](i.rectify.md)*,
*[i.ortho.photo](i.ortho.photo.md)* and others ask the user for the name
of an imagery group whose data are to be analyzed. Imagery analysis
programs like *[i.cluster](i.cluster.md)* and *[i.maxlik](i.maxlik.md)*
ask the user for the imagery group and imagery subgroup whose data are
to be analyzed.

## NOTES

The *i.group* options are only available for imagery map layers in the
current LOCATION_NAME.

Subgroup names may not contain more than 12 characters.

### EXAMPLE

This example runs in the "landsat" mapset of the North Carolina sample
dataset. The following command creates a group and subgroup containing
only the visible light bands of Landsat-7:

```sh
i.group group=vis_bands subgroup=vis_bands input=lsat7_2000_10,lsat7_2000_20,lsat7_2000_30
```

To list files in the `vis_bands` subgroup under the `vis_bands` group:

```sh
# In shell format
i.group group=vis_bands subgroup=vis_bands -l format=shell
lsat7_2000_10@PERMANENT
lsat7_2000_20@PERMANENT
lsat7_2000_30@PERMANENT

# In JSON format
i.group group=vis_bands subgroup=vis_bands -l format=json
{
    "group": "vis_bands",
    "subgroup": "vis_bands",
    "maps": [
        "lsat7_2000_10@PERMANENT",
        "lsat7_2000_20@PERMANENT",
        "lsat7_2000_30@PERMANENT"
    ]
}
```

To list maps in the `vis_bands` group using Python:

```python
import grass.script as gs

# Run i.group with JSON output and the -l flag
data = gs.parse_command(
    "i.group",
    group="vis_bands",
    flags="l",
    format="json",
)

for item in data["maps"]:
    print(item)
```

```text
lsat7_2000_10@PERMANENT
lsat7_2000_20@PERMANENT
lsat7_2000_30@PERMANENT
```

To list subgroups in the `vis_bands` group:

```sh
# In shell format
i.group group=vis_bands -s format=shell
vis_bands

# In JSON format
i.group group=vis_bands -s format=json
{
    "group": "vis_bands",
    "subgroups": [
        "vis_bands"
    ]
}
```

## SEE ALSO

The GRASS 4 *[Image Processing
manual](https://grass.osgeo.org/gdp/imagery/grass4_image_processing.pdf)*

*[g.gui.gcp](g.gui.gcp.md), [i.cluster](i.cluster.md),
[i.maxlik](i.maxlik.md), [i.rectify](i.rectify.md),
[i.ortho.photo](i.ortho.photo.md)*

## AUTHORS

Michael Shapiro, U.S.Army Construction Engineering Research Laboratory  
Parser support: Bob Covill (Tekmap, Canada)
