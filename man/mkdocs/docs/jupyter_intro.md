---
authors:
    - Corey T. White
    - GRASS Development Team
---

# Jupyter Notebooks

The `grass.jupyter` Python package provides a [Jupyter](https://jupyter.org/)
notebook interface to GRASS. The package contains modules for creating map figures,
interactive web maps, visualize series and time series datasets, and 3D visualizations.

To get started with `grass.jupyter`, import the package:

```python
import grass.jupyter as gj
```

This will give you access to all of the modules and classes in the `grass.jupyter`.
From here, you can create a GRASS session with the `gj.init` function:

```python
# Start a GRASS session
session = gj.init("project_directory")
```

The `gj.init` function will create a new GRASS session for the specified project
and mapset. If you need to switch to a different mapset, you can use the `gj.switch_mapset`
function:

```python
session.switch_mapset("mapset_name")
```

## Map

The `gj.Map` class in `grass.jupyter` provides a way to create static maps in Jupyter.
Here we create a map of the elevation raster and save it to a file:

```python
# Create a new map, set the dimensions, and save the map to a file
m = gj.Map()

# Add a raster map to the map object
m.d_rast(map="elevation")

# Add a vector map to the map object
m.d_vect(map="roadsmajor", color="black")

# Display the map
m.show()
```

!!! grass-tip "Reserved Keywords"
    Avoid using the reserved keyword `map` as a variable name in Python.  
    :x: `map = gj.Map()`  
    :white_check_mark: `m = gj.Map()`

In addition to displaying raster and vector maps, the `gj.Map` can access many
of the [display tool](display.md) in GRASS. For a complete list of cartographic features
you can refer to the [Cartography](topic_cartography.md) topic page.

For example, let's add a legend, barscale, and shaded relief to the map:

```python
# Create a map object
m = gj.Map()

# Add a shaded relief map
m.d_shade(color="elevation", shade="aspect")

# Add the vector map to the map object
m.d_vect(map="roadsmajor", color="black")

# Add a raster legend
m.d_legend(
    raster="elevation",
    at=(5,40,11,14),
    title="Elevation (m)",
    font="sans",
    flags="b"
)

# Add a scale bar to the map
m.d_barscale(at=(50,7), flags="n")

# Display the map
m.show()
```

!!! grass-tip "Order Matters"
    <!-- markdownlint-disable-next-line MD046 -->
    Map features are added to the map in the order they are called. For example,
    if you add a raster map and then a vector map, the vector map will be drawn
    on top of the raster map.

In fact the `gj.Map` object doesn't even need to be a map! You can use other
display tools like [d.histogram](d.histogram.md) or
[d.polar](d.polar.md), to create plots in Jupyter:

```python
hist_plot = gj.Map()
hist_plot.d_histogram(map="elevation")
hist_plot.show()
```

```python
polar_plot = gj.Map()
polar_plot.d_polar(map="aspect", undef=0)
polar_plot.show()
```

## Interactive Maps

The `gj.InteractiveMap` class in `grass.jupyter` provides a way to create interactive
web maps in Jupyter. Interactive maps are created using the
[ipyleaflet](https://ipyleaflet.readthedocs.io/en/latest/)
or [folium](https://python-visualization.github.io/folium/) libraries.
The default is [ipyleaflet](https://ipyleaflet.readthedocs.io/en/latest/),
which gives you more interactivity and control over the map. Here we create
an interactive map of the aspect and elevation rasters and the roadsmajor
vector map:

```python
# Create an interactive map
m = gj.InteractiveMap()
m.add_raster("aspect", opacity=0.5)
m.add_raster("aspect", opacity=0.7)
m.add_raster("elevation", opacity=0.7)
m.add_vector("roadsmajor")
m.show()
```

The map gives you the ability to query the map, zoom in and out, and pan around,
set the computational region, and add vector data to the map.

## 3D Visualization

```python
elevation_3dmap = gj.Map3D()
elevation_3dmap.render(
    elevation_map="elevation",
    color_map="landuse",
    perspective=35,
    height=5000,
    resolution_fine=1,
    zexag=5,
    fringe=['ne','nw','sw','se'],
    fringe_elevation=10,
    arrow_position=[100,50],
)
elevation_3dmap.overlay.d_barscale(at=(60,10), flags="")
elevation_3dmap.overlay.d_legend(raster="landuse", at=(5,35,5,9), flags="b")
elevation_3dmap.show()
```

The parameters are the same as parameters of
the [m.nviz.image](m.nviz.image.md) tool.

## Series Maps

```python
m = gj.SeriesMap()
m.add_rasters(["elevation", "slope", "aspect"])
m.add_vectors(["roads", "buildings"])
m.d_barscale(at=(80, 10))
m.show()
m.save("series_map.gif")
```

!!! grass-tip "Use Regions"
    <!-- markdownlint-disable-next-line MD046 -->
    Set the zoom level based on the computational region of the map by setting
    `use_region=True` in the `grass.jupyter` class.

## Time Series

The `gj.TimeSeriesMap` class in `grass.jupyter` provides a way visualize
GRASS' [space time datasets](temporalintro.md) in Jupyter. Here we create a time
series map of flood inundation extent at different depths:

```python
flood_map = gj.TimeSeriesMap()

# Add the base map
flood_map.d_rast(map="naip_2022_rgb")
flood_map.d_vect(map="ncssm", fill_color="none", color="white", width=2)
flood_map.d_vect(map="roads")

# Add the time series data
flood_map.add_raster_series("flooding")

# Add map features
flood_map.d_legend()

#Display the map
flood_map.show()
```

!!! grass-tip "Render Large Time Series"
    <!-- markdownlint-disable-next-line MD046 -->
    Large time series can take a long time to render. To speed things up
    you can use the `m.render()` function to render the PNG files to a
    temporary directory and then use the `m.save()` or `m.show()` to display
    the time series.

## Integration with Other Python Libraries

The `grass.jupyter` package can be used in conjunction with other Python libraries
like [pandas](https://pandas.pydata.org/), [numpy](https://scipy.org/),
[matplotlib](https://matplotlib.org/), [seaborn](https://seaborn.pydata.org/),
and many more.

Here we will use the [r.profile](r.profile.md) tool to extract elevation values
across a transect of the elevation raster as JSON data. We will then load the
JSON data into a pandas DataFrame and create a scatter plot of
distance vs. elevation with color coding:

```python
import grass.script as gs
import pandas as pd
import matplotlib.pyplot as plt

# Run r.profile command
elevation = gs.parse_command(
    "r.profile",
    input="elevation",
    coordinates="641712,226095,641546,224138,641546,222048,641049,221186",
    format="json",
    flags="gc"
)

# Load the JSON data into a DataFrame
df = pd.DataFrame(elevation)

# Convert the RGB color values to hex format for Matplotlib
df["color"] = df.apply(
        lambda x: "#{:02x}{:02x}{:02x}".format(
            int(x["red"]),
            int(x["green"]),
            int(x["blue"])
        ),
        axis=1
    )

# Create the scatter plot
plt.figure(figsize=(10, 6))
plt.scatter(df['distance'], df['elevation'], c=df['color'], marker='o')
plt.title('Profile of Distance vs. Elevation with Color Coding')
plt.xlabel('Distance (meters)')
plt.ylabel('Elevation')
plt.grid(True)
plt.show()
```

*Example from [r.profile](r.profile.md) tool*

## Python Library Documentation

For complete documentation on the `grass.jupyter` package, see the
[grass.jupyter](https://grass.osgeo.org/grass-stable/manuals/libpython/grass.jupyter.html)
library documentation page.

## Tutorials

- [Get started with GRASS in Jupyter Notebooks on Windows](https://grass-tutorials.osgeo.org/content/tutorials/get_started/JupyterOnWindows_OSGeo4W_Tutorial.html)
- [Get started with GRASS & Python in Jupyter Notebooks (Unix/Linux)](https://grass-tutorials.osgeo.org/content/tutorials/get_started/fast_track_grass_and_python.html)
- [Get started with GRASS GIS in Google Colab](https://grass-tutorials.osgeo.org/content/tutorials/get_started/grass_gis_in_google_colab.html)
