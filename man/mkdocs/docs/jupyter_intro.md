---
authors:
    - Corey T. White
    - GRASS Development Team
---

# Jupyter Notebooks

The `grass.jupyter` Python package provides a [Jupyter](https://jupyter.org/)
notebook interface to GRASS. It includes modules for creating map figures,
interactive web maps, visualizing data series and time series, and generating
3D visualizations.

If you don't have a project yet, create a new one first:

```python
import grass.script as gs

gs.create_project("path/to/my_project", epsg=3358)
```

To get started with `grass.jupyter`, import the package,
and start a GRASS session with the `gj.init` function:

```python
import grass.jupyter as gj

session = gj.init("path/to/my_project")
```

Now you can import raster or vector data with [r.import](r.import.md)
and [v.import](v.import.md).

!!! grass-tip "Mapsets"
    If not specified otherwise in the `gj.init` function, the session will
    start in the default
    mapset (subproject) of a project. If you need to switch to a different mapset,
    you can use the `gj.switch_mapset` function.

## Map

The `gj.Map` class in `grass.jupyter` provides a way to create static maps in Jupyter.
Here we create a map of the elevation raster overlayed with roads vector map:

```python
# Create a new map
m = gj.Map()

# Add a raster map to the map object
m.d_rast(map="elevation")

# Add a vector map to the map object
m.d_vect(map="roadsmajor", color="black")

# Display the map
m.show()
```

!!! grass-tip "Order Matters"
    <!-- markdownlint-disable-next-line MD046 -->
    Map features are added to the map in the order they are called. For example,
    if you add a raster map and then a vector map, the vector map will be drawn
    on top of the raster map.

In addition to displaying raster and vector maps, the `gj.Map` can access many
of the [display tools](display.md) in GRASS. For a complete list of cartographic
features you can refer to the [Cartography](topic_cartography.md) topic page.

For example, let's add a legend, barscale, and shaded relief to the map:

```python
m = gj.Map()

# Add a shaded relief map
m.d_shade(color="elevation", shade="aspect")

# Add a raster legend
m.d_legend(
    raster="elevation",
    at=(5, 40, 11, 14),
    title="Elevation (m)",
    font="sans",
    flags="b"
)

# Add a scale bar to the map
m.d_barscale(at=(50,7), flags="n")

# Display the map
m.show()
```

!!! grass-tip "Map Size and Extent"
    See [detailed documentation](https://grass.osgeo.org/grass-stable/manuals/libpython/grass.jupyter.html#module-grass.jupyter.map)
    for changing the map size and geographic extent.

## Interactive Maps

The `gj.InteractiveMap` class provides a way to create interactive
web maps in Jupyter. Interactive maps are created using the
[ipyleaflet](https://ipyleaflet.readthedocs.io/en/latest/)
or [folium](https://python-visualization.github.io/folium/) libraries.
The default is [ipyleaflet](https://ipyleaflet.readthedocs.io/en/latest/),
which gives you more interactivity and control over the map. Here we create
an interactive map of the elevation raster and the roadsmajor
vector map:

```python
# Create an interactive map
m = gj.InteractiveMap()
m.add_raster("elevation", opacity=0.7)
m.add_vector("roadsmajor")
m.show()
```

The map gives you the ability to query the map, zoom in and out, and pan around,
set the computational region, and create simple vector data by digitizing.
To change the default basemap, see [documentation](https://grass.osgeo.org/grass-stable/manuals/libpython/grass.jupyter.html#module-grass.jupyter.interactivemap).

## 3D Visualization

The `gj.Map3D` class creates 3D visualizations as static images.

```python
elevation_3dmap = gj.Map3D()
elevation_3dmap.render(
    elevation_map="elevation",
    color_map="landuse",
    perspective=35,
    height=5000,
    resolution_fine=1,
    zexag=5
)
elevation_3dmap.overlay.d_legend(raster="landuse", at=(5, 35, 5, 9), flags="b")
elevation_3dmap.show()
```

The parameters of the `render()` function are the same as parameters of the
[m.nviz.image](m.nviz.image.md) tool, which is used in the background.

## Series Maps

The `gj.SeriesMap` class animates a series of maps, allowing users to slide between
maps and play a continuous loop.

```python
m = gj.SeriesMap()
m.add_rasters(["elevation", "slope", "aspect"])
m.d_vect(map="roads")
m.d_barscale(at=(80, 10))
m.show()
# Save the map as animated gif
m.save("series_map.gif")
```

## Time Series

The `gj.TimeSeriesMap` class provides a way to visualize
GRASS' [space time datasets](temporalintro.md) in Jupyter. Here we create a time
series map of flood inundation extent at different depths:

```python
flood_map = gj.TimeSeriesMap()

# Add the base map
flood_map.d_rast(map="elevation")
flood_map.d_vect(map="roadsmajor")

# Add the time series data
flood_map.add_raster_series("flooding")

# Add map features
flood_map.d_legend()

#Display the map
flood_map.show()
```

## Python Library Documentation

For complete documentation on the `grass.jupyter` package, see the
[grass.jupyter](https://grass.osgeo.org/grass-stable/manuals/libpython/grass.jupyter.html)
library documentation page.

## Tutorials

- [Get started with GRASS in Jupyter Notebooks on Windows](https://grass-tutorials.osgeo.org/content/tutorials/get_started/JupyterOnWindows_OSGeo4W_Tutorial.html)
- [Get started with GRASS & Python in Jupyter Notebooks (Unix/Linux)](https://grass-tutorials.osgeo.org/content/tutorials/get_started/fast_track_grass_and_python.html)
- [Get started with GRASS GIS in Google Colab](https://grass-tutorials.osgeo.org/content/tutorials/get_started/grass_gis_in_google_colab.html)
