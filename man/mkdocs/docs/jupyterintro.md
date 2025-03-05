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
m.d_rast(map="elevation")
m.d_vect(map="roads", color="black")
m.show()
```

![Static Map](static_map.png)
*Elevation Map*

!!! tip "Reserved Keywords"
    <!-- markdownlint-disable-next-line MD046 MD033-->
    Avoid using the reserved keyword `map` as a variable name in Python.<br>
    <!-- markdownlint-disable-next-line MD046 MD033 -->
    :x: `map = gj.Map()`<br>
    <!-- markdownlint-disable-next-line MD046 -->
    :white_check_mark: `m = gj.Map()`

In addition to displaying raster and vector maps, the `gj.Map` can access many
of the [display tool](display.md) in GRASS. For example, let's add a legend,
barscale, and shaded relief to the map:

```python
m = gj.Map(height=600, width=800, filename="relief_map.png")
m.d_shade(shade="relief", color="elevation", brighten=30)
m.d_vect(map="roads", color="black")
m.d_legend(raster="elevation", at=(80, 90))
m.d_barscale(at=(80, 10), flags="n")
m.show()
```

![Static Map](relief_map.png)
*Shaded Relief Elevation Map*

!!! grass-tip "Order Matters"
    <!-- markdownlint-disable-next-line MD046 -->
    Map features are added to the map in the order they are called. For example,
    if you add a raster map and then a vector map, the vector map will be drawn
    on top of the raster map.

In fact the `gj.Map` object doesn't even need to be a map! You can use other
display tools like [d.histogram](display.md#d.histogram), to create a histogram:

```python
m = gj.Map(filename="histogram.png")
m.d_histogram(raster="elevation")
m.show()
```

![Histogram](histogram.png)
*Elevation Histogram*

## Interactive Maps

```python
# Create an interactive map
m = gj.InteractiveMap()
m.add_raster("elevation", opacity=0.7)
m.add_vector("roads")
m.add_layer_control()
m.show()
```

## Series Maps

```python
m = gj.SeriesMap(height = 500)
m.add_rasters(["elevation", "slope", "aspect"])
m.add_vectors(["roads", "buildings"])
m.d_barscale(at=(80, 10))
m.show()
m.save("series_map.gif")
```

## Time Series

The `gj.TimeSeriesMap` submodule in `grass.jupyter` provides a way visualize
GRASS' [space time datasets](temporalintro.md) in Jupyter. Here we create a time
series map of the...:

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

## 3D Visualization

## Python Library Documentation

[grass.jupyter](https://grass.osgeo.org/grass-stable/manuals/libpython/grass.jupyter.html)
