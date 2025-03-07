---
authors:
    - Corey T. White
    - GRASS Development Team
---

# Interfaces

GRASS provides a number of interfaces for interacting with the software.
The most common interfaces are command line, Python, Jupyter Notebooks,
and graphical user interface.

## Command Line

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

To learn more about the command line interface, see the
[Command Line Interface](command_line_intro.md) page.

## Python

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
[grass.script package](python_intro.md) page.

## Jupyter Notebooks

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
[Jupyter Notebooks](jupyter_intro.md) page.

## Desktop Graphical User Interface

The GRASS Desktop GUI is a graphical user interface for GRASS. The GUI provides
a visual interface for interacting with GRASS. The GUI provides a number of tools
for interactive data processing, analysis, and visualization. The GUI is
avaliable on Windows, macOS, and Linux.

Learn more about the GRASS Desktop GUI on the
[Desktop GUI](wxguiintro.md) page.
