---
authors: 
    - Corey T. White
    - GRASS Development Team
title: Jupyter Notebooks
---

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
m = gj.Map(height=600, width=800, filename="static_map.png")

# Add a raster map to the map object
m.d_rast(map="elevation")

# Add a vector map to the map object
m.d_vect(map="roadsmajor", color="black")

# Display the map
m.show()
```

![Static Map](jupyterintro_static_map.png)
*Elevation Map*

!!! grass-tip "Reserved Keywords"
    <!-- markdownlint-disable-next-line MD046 MD033-->
    Avoid using the reserved keyword `map` as a variable name in Python.<br>
    <!-- markdownlint-disable-next-line MD046 MD033 -->
    :x: `map = gj.Map()`<br>
    <!-- markdownlint-disable-next-line MD046 -->
    :white_check_mark: `m = gj.Map()`

In addition to displaying raster and vector maps, the `gj.Map` can access many
of the [display tool](display.md) in GRASS. For a complete list of cartographic features
you can refer to the [Cartography](topic_cartography.md) topic page.

For example, let's add a legend, barscale, and shaded relief to the map:

```python
# Create a map object
m = gj.Map(height=600, width=800, filename="relief_map.png")

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

![Shaded Releif Map](jupyterintro_relief_map.png)
*Shaded Relief Elevation Map*

!!! grass-tip "Order Matters"
    <!-- markdownlint-disable-next-line MD046 -->
    Map features are added to the map in the order they are called. For example,
    if you add a raster map and then a vector map, the vector map will be drawn
    on top of the raster map.

In fact the `gj.Map` object doesn't even need to be a map! You can use other
display tools like [d.histogram](d.histogram) or
[d.polar](d.polar.md), to create plots in Jupyter:

```python
hist_plot = gj.Map(filename="histogram.png")
hist_plot.d_histogram(map="elevation")
hist_plot.show()
```

![Histogram](jupyterintro_hist_plot.png)
*Elevation Histogram*

```python
polar_plot = gj.Map(font="sans", height=600, width=800)
polar_plot.d_polar(map="aspect", undef=0)
polar_plot.show()
```

![Polar Plot](jupyterintro_polar_plot.png)
*Aspect Polar Plot*

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
m = gj.InteractiveMap(height=600, width=800)
m.add_raster("aspect", opacity=0.5)
m.add_raster("aspect", opacity=0.7)
m.add_raster("elevation", opacity=0.7)
m.add_vector("roadsmajor")
m.show()
```

![Interactive Map](jupyterintro_interactive.png)
*Interactive Web Map (ipyleafet)*

The map gives you the ability to query the map, zoom in and out, and pan around,
set the compuational region, and add vector data to the map.

## 3D Visualization

```python
elevation_3dmap = gj.Map3D(width=800, height=600)
# Full list of options m.nviz.image
# https://grass.osgeo.org/grass-stable/manuals/m.nviz.image.html
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

![3D Map](jupyterintro_3D_map.png)
*Shaded Relief Elevation Map*

## Series Maps

```python
m = gj.SeriesMap(height=600, width=800)
m.add_rasters(["elevation", "slope", "aspect"])
m.add_vectors(["roads", "buildings"])
m.d_barscale(at=(80, 10))
m.show()
m.save("series_map.gif")
```

![Series Map](jupyterintro_series_map.gif)

## Time Series

The `gj.TimeSeriesMap` class in `grass.jupyter` provides a way visualize
GRASS' [space time datasets](temporalintro.md) in Jupyter. Here we create a time
series map of flood inundation extent at different depths:

```python
m = gj.TimeSeriesMap("series_name", height=600, width=800)
m.d_legend()
m.show()
m.save("change.gif")
```

!!! grass-tip "Render Large Time Series"
    <!-- markdownlint-disable-next-line MD046 -->
    Large time series can take a long time to render. To speed things up
    you can use the `m.render()` function to render the png files to a
    temporary directory and then use the `m.save()` or `m.show()` to display
    the time series.

## Integration with Other Python Libraries

The `grass.jupyter` package can be used in conjunction with other Python libraries
like [pandas](https://pandas.pydata.org/), [numpy](https://scipy.org/),
[matplotlib](https://matplotlib.org/), [seaborn](https://seaborn.pydata.org/),
and many more.

## Python Library Documentation

[grass.jupyter](https://grass.osgeo.org/grass-stable/manuals/libpython/grass.jupyter.html)

## Tutorials

- [Get started with GRASS in Jupyter Notebooks on Windows](https://grass-tutorials.osgeo.org/content/tutorials/get_started/JupyterOnWindows_OSGeo4W_Tutorial.html)
- [Get started with GRASS & Python in Jupyter Notebooks (Unix/Linux)](https://grass-tutorials.osgeo.org/content/tutorials/get_started/fast_track_grass_and_python.html)
- [Get started with GRASS GIS in Google Colab](https://grass-tutorials.osgeo.org/content/tutorials/get_started/grass_gis_in_google_colab.html)
