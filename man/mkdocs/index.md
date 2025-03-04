---
authors: 
    - Corey T. White
    - GRASS Development Team
title: Getting Started
---

## What is GRASS?

[GRASS](https://grass.osgeo.org/) is a geosptial processing engine for
advance analysis and visualization of geospatial data. It is a powerful tool for
processing and analyzing geospatial data sets. GRASS is a free and open source
software, released under an open source [GNU GPLed](https://www.gnu.org/licenses/gpl.html).

Downloaded and installed GRASS here.

<!-- markdownlint-disable-next-line MD013 -->
[:material-download: Download and Install](https://grass.osgeo.org/download/){ .md-button }

## Interfaces

GRASS provides a number of interfaces for interacting with the software. The
most common interfaces are:

### Terminal

The terminal interface allows you to start a GRASS session to run GRASS
commands, execute scripts, or open the GUI.

Here we creating a new project for the NAD83(HARN)/North Carolina coordinate
reference system (EPSG:3358) and start a GRASS session in the terminal.

```sh
grass -c EPSG:3358 {project directory} --text
```

The terminal can now execute GRASS commands.

```sh
g.region raster=elevation
r.slope.aspect elevation=elevation slope=slope aspect=aspect
```

To learn more about the terminal interface, see the
[Terminal Interface](terminalintro.md) page.

### Python

The `grass.script` module provides a Python interface to GRASS. This allows
users to write Python scripts to interact with GRASS. The `grass.script` module
contains the `gs.script.core` module which provides the core functionality for the
GRASS Python interface, the `grass.script.raster` module which provides
functionality for working with raster data, and the `grass.script.vector` module
which provides functionality for working with vector data.

To get started Python, create a new project with `gs.create_project` and start a
GRASS session with the `grass.script.setup.init` function to initialize the
GRASS environment.

```python
import sys
import subprocess
from pathlib import Path

# Append GRASS to the python system path
sys.path.append(
    subprocess.check_output(["grass", "--config", "python_path"], text=True).strip()
)

import grass.script as gs

# Create a new project
gs.create_project(path=grassdata, name=project_name, epsg="3358", overwrite=False)

# Initialize the GRASS session
with gs.setup.init(Path("grassdata/project_name")) as session:

    # Run GRASS commands
    gs.run_command('g.region', raster='elevation')
    gs.run_command('r.slope.aspect', elevation='elevation', slope='slope', aspect='aspect')
```

To learn more about the GRASS Python Scripting library, see the
[grass.script package](pythonintro.md) page.

### Jupyter Notebooks

Jupyter Notebooks are a great way to interact with GRASS. The `grass.jupyter`
module provides a Jupyter interface to GRASS. This allows users to write Jupyter
Notebooks to interact with GRASS. The `grass.jupyter` module contains the `Map`,
`InteractiveMap`, `Map3D`, `TimeSeriesMap`, and `SeriesMap` classes which
provide functionality for working with maps in Jupyter Notebooks.

```python
from pathlib import Path
import grass.jupyter as gj

session = gj.init(Path(grassdata, project_name))

slope_map = gj.Map()  # Create a new map
slope_map.d_rast(map='slope')  # Add the slope raster to the map
slope_map.d_barscale(at=(80, 10))  # Add a bar scale to the map
slope_map.d_legend(raster='slope', at=(80, 90))  # Add a legend to the map
slope_map.show()  # Display the map
```

![Slope Map](r_slope_aspect_slope.png)

Learn more about the GRASS Jupyter interface on the
[Jupyter Notebooks](jupyterintro.md) page.

### [Desktop GUI](wxguiintro.md)

The GRASS Desktop GUI is a graphical user interface for GRASS. The GUI provides
a visual interface for interacting with GRASS. The GUI provides a number of tools
for interactive data processing, analysis, and visualization. The GUI is
avaliable on Windows, macOS, and Linux.

Learn more about the GRASS Desktop GUI on the
[Desktop GUI](wxguiintro.md) page.

## Processing Tools

GRASS provide a wide range of tools for geospatial processing, modeling,
analysis, and visualization. The tools are organized into categories based
on the type of data they process. The tools are prefixed with a letter to
indicate the type of data they process.

The following table provides a list of the prefixes and the categories they represent.
Follow the topics links to see a complete list of tools in each category. Or try
using the search feature to find a specific tool or topic by subject
matter (e.g. hydrology, landscapes).

| Prefix | Category                         | Description                        | Topic                                      |
|--------|----------------------------------|------------------------------------|-------------------------------------------|
| `g.`   | General                          | General GIS management tools       | [General Tools](general.md)               |
| `r.`   | Raster                           | Raster data processing tools       | [Raster Tools](raster.md)                 |
| `r3.`  | 3D Raster                        | 3D Raster data processing tools    | [3D Raster Tools](raster3d.md)            |
| `v.`   | Vector                           | Vector data processing tools       | [Vector Tools](vector.md)                 |
| `i.`   | Imagery                          | Imagery processing tools           | [Imagery Tools](imagery.md)               |
| `t.`   | Temporal                         | Temporal data processing tools     | [Temporal Tools](temporal.md)             |
| `db.`  | Database                         | Database management tools          | [Database Tools](database.md)             |
| `d.`   | Display                          | Display and visualization tools    | [Display Tools](display.md)               |
| `m.`   | Miscellaneous                    | Miscellaneous tools                | [Miscellaneous Tools](miscellaneous.md)   |
| `ps.`  | Postscript                       | Postscript tools                   | [Postscript Tools](postscript.md)         |

## Development

GRASS is an open source project and welcomes contributions from the community.
The GRASS Development documentation provides information on how to get started
developing for GRASS, how to contribute to the project, and how to get involved
with the GRASS community.

For more information on developing for GRASS, see the
[Development Introduction](developmentintro.md) page.
