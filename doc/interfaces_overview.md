---
authors:
    - Corey T. White
    - GRASS Development Team
---

# Interfaces overview

GRASS provides a number of interfaces for interacting with the software.
The most common interfaces are command line, Python, Jupyter Notebooks,
and graphical user interface.

## Command line

The command line, also know as terminal or shell, interface allows you to start
a GRASS session to run GRASS commands, execute scripts, or open the graphical
user interface.

Here we create a new project for the NAD83(HARN)/North Carolina coordinate
reference system (EPSG:3358) and start a GRASS session as a new shell in
your terminal:

```sh
grass -c EPSG:3358 {project directory} --text
```

The shell can now execute GRASS commands.

```sh
g.region raster=elevation
r.slope.aspect elevation=elevation slope=slope aspect=aspect
```

[Learn more :material-arrow-right-bold:](command_line_intro.md){ .md-button }

## Python

GRASS Python interface provides libraries to create GRASS scripts and access
the internal data structures of GRASS. The Python interface consists of
two main libraries:
*[grass.script](https://grass.osgeo.org/grass-stable/manuals/libpython/script_intro.html)*
provides a Python interface to GRASS tools
and *[grass.pygrass](https://grass.osgeo.org/grass-stable/manuals/libpython/pygrass_index.html)*
enables access to the internal data structures of GRASS.

To get started with scripting, create a new project with `gs.create_project` and
start a GRASS session with the `gs.setup.init` function to initialize the
GRASS environment.

```python
import sys
import subprocess

# Append GRASS to the python system path
sys.path.append(
    subprocess.check_output(["grass", "--config", "python_path"], text=True).strip()
)

import grass.script as gs

# Create a new project
gs.create_project(path="path/to/my_project", epsg="3358")

# Initialize the GRASS session
with gs.setup.init("path/to/my_project") as session:

    # Run GRASS tools
    gs.run_command("r.import", input="/path/to/elevation.tif", output="elevation")
    gs.run_command("g.region", raster="elevation")
    gs.run_command("r.slope.aspect", elevation="elevation", slope="slope")
```

[Learn more :material-arrow-right-bold:](python_intro.md){ .md-button }

## Jupyter notebooks

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

[Learn more :material-arrow-right-bold:](jupyter_intro.md){ .md-button }

## Desktop graphical user interface

The GRASS Desktop GUI is a graphical user interface for GRASS.
Designed for efficiency and ease of use, it provides an intuitive way to
interact with spatial data and the powerful tools available in GRASS.
The GUI supports the visualisation of spatial data,
the execution of geoprocessing tasks and the management of complex workflows.

![GRASS desktop GUI](grass_start.png)

[Learn more :material-arrow-right-bold:](helptext.md){ .md-button }
