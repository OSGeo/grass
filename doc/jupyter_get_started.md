---
authors:
    - Anna Petrasova
    - GRASS Development Team
---

# Get started with Jupyter notebooks

Jupyter notebooks provide an interactive environment for writing and running code,
combining text, code, and visualizations in a single document.
They are widely used for data analysis, workflow prototyping, and scientific computing,
making them a powerful tool for working with GRASS.

The `grass.jupyter` Python package provides a [Jupyter](https://jupyter.org/)
notebook interface to GRASS. It includes modules for creating map figures,
interactive web maps, visualizing data series and time series, and generating
3D visualizations.

To get started with `grass.jupyter`, import the package,
and start a GRASS session with the `gj.init` function:

```python
import grass.jupyter as gj

session = gj.init("path/to/my_project")
```

All classes and functions for interaction in notebooks are now available under `gj`,
for example we can display a map with a selected raster and vector:

```python
# Create a new map
m = gj.Map()

# Add a raster map to the map object
m.d_rast(map="elevation")

# Add a vector map to the map object
m.d_vect(map="streets", color="black")

# Display the map
m.show()
```

![Elevation map overlayed with streets with gj.Map](jupyter_map.png)

Continue exploring the `grass.jupyter` package capabilities [with more examples](jupyter_intro.md)
or run the Jupyter tutorial on Binder:

[![Binder](https://mybinder.org/badge_logo.svg)](https://mybinder.org/v2/gh/OSGeo/grass/main?labpath=doc%2Fexamples%2Fnotebooks%2Fjupyter_tutorial.ipynb)

## Python library documentation

For complete documentation on the `grass.jupyter` package, see the
[grass.jupyter](https://grass.osgeo.org/grass-stable/manuals/libpython/grass.jupyter.html)
library documentation page.

## Tutorials

- [Get started with GRASS in Jupyter Notebooks on Windows](https://grass-tutorials.osgeo.org/content/tutorials/get_started/JupyterOnWindows_OSGeo4W_Tutorial.html)
- [Get started with GRASS & Python in Jupyter Notebooks (Unix/Linux)](https://grass-tutorials.osgeo.org/content/tutorials/get_started/fast_track_grass_and_python.html)
- [Get started with GRASS in Google Colab](https://grass-tutorials.osgeo.org/content/tutorials/get_started/grass_gis_in_google_colab.html)
