---
authors: 
    - Corey T. White
    - GRASS Development Team
title: Python Introduction
---

GRASSs' Python interface provides libraries to create GRASS scripts and access
the internal data structures of GRASS. The Python interface is broken down into
three main libraries:
[grass.script](https://grass.osgeo.org/grass-stable/manuals/libpython/script.html#module-script.core),
[grass.pygrass](https://grass.osgeo.org/grass-stable/manuals/libpython/pygrass_index.html),
and [grass.temporal](https://grass.osgeo.org/grass-stable/manuals/libpython/temporal_framework.html).
The `grass.script` library provides a Python interface to GRASS, the `grass.pygrass`
library provides access to the internal data structures of GRASS, and the
`grass.temporal` library provides a Python interface to the temporal framework
in GRASS.

Full Documentation: [GRASS Python Library](https://grass.osgeo.org/grass-stable/manuals/libpython/index.html)

## GRASS Scripts

To get started with the GRASS Python interface, you must first append the GRASS
Python path to the system path and then import the `grass.script` library. From
here, you can create a new project with `gs.create_project` and start a GRASS
session with the `grass.script.setup.init` function to initialize the GRASS
environment.

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
    gs.run_command(
        'r.slope.aspect',
        elevation='elevation',
        slope='slope',
        aspect='aspect'
    )
```

!!! grass-tip "Use `if __name__ == '__main__':`"
    <!-- markdownlint-disable-next-line MD046 -->
    When writing GRASS scripts, it is a good idea to include the GRASS session
    setup in a `if __name__ == '__main__':` block. This will allow the script to
    be imported into other scripts without running the GRASS session setup or executed
    as a standalone script.

The `gs.run_command`, `gs.read_command`, `gs.write_command`, and
`gs.parse_command` functions are the four main `grass.scripts`
functions used to execute GRASS tools with Python.

!!! grass-tip "Command Syntax"
    <!-- markdownlint-disable-next-line MD046 -->
    The syntax for the `gs.run_command`, `gs.read_command`, `gs.write_command`,
    and `gs.parse_command` function follows the pattern:
    `function_name(*module.name*, option1=*value1*, option2=*...*, flags=*'flagletters'*)`

### run_command

The `gs.run_command` function is used to run a GRASS command when you require
no return value from the command. For example, when setting the computational region
with `g.region`:

```python
gs.run_command('g.region', raster='elevation')
```

### read_command

The `gs.read_command` function is used to run a GRASS command when you require
the results sent to the `stdout`. For example, when reading the output of `r.univar`:

```python
stats = gs.read_command('r.univar', map='elevation', flags='g')
```

### write_command

The `gs.write_command` function is used to run send data to a GRASS command
through the `stdin`. For example, when writing a custom color scheme to
a raster map with `r.colors`:

```python
color_scheme = """
20% #ffffd4
40% #fed98e
60% #fe9929
80% #d95f0e
100% #993404
"""
gs.write_command('r.colors',
    map='elevation',
    rules='-',  # Read from stdin
    stdin=color_scheme
)
```

### parse_command

The `gs.parse_command` function is used to run a GRASS command when you require
the results sent to the `stdout` and parsed as a key-value pair. For example,
to parse the output of `r.univar` as JSON data:

```python
univar_json = gs.parse_command(
    'r.univar',
    map='elevation',
    format="json",
    flags='g'
)
```

Full Documentation: [GRASS Python Library](https://grass.osgeo.org/grass-stable/manuals/libpython/script_intro.html)

## PyGRASS

PyGRASS is a Python library that provides access to the internal data structures
of GRASS for more advanced scripting and modeling. PyGRASS works directly with the
C libraries of GRASS and providing a Pythonic interface. The core packages of
PyGRASS include the [gis](https://grass.osgeo.org/grass-stable/manuals/libpython/pygrass_gis.html),
[raster](https://grass.osgeo.org/grass-stable/manuals/libpython/pygrass_raster.html)
and [vector](https://grass.osgeo.org/grass-stable/manuals/libpython/pygrass_vector.html)
data access, and [modules](https://grass.osgeo.org/grass-stable/manuals/libpython/pygrass_modules.html).

- gis: Project and Region Management
- raster: Raster Data Access
- vector: Vector Data Access
- modules: GRASS Tool Access

For a complete reference of the PyGRASS library, see the Full Documentation:
[PyGRASS Library](https://grass.osgeo.org/grass-stable/manuals/libpython/pygrass_index.html)

### Project and Region Management

#### Project Management

The `grass.pygrass.gis` module provides access to the project and region management
of GRASS. The core classes include `Gisdbase`, `Location`, `Mapset`,
and `VisibleMapset`. The `Gisdbase` class provides access to the GRASS database
and where you can manage GRASS projects and mapsets.

For example, to list all projects in your GRASS database directory you can use:

```python
from grass.pygrass import gis

grassdata = gis.Gisdbase()
projects = grassdata.locations()
print(projects)
```

This will return a list of all projects in the GRASS database directory as
`Location` objects. The `Location` object provides access to the specific project
and its mapsets.

```text
['nc_spm_08_grass7', 'my_project']
```

For more details about the `gis` module, see the Full Documentation:
[GIS Module](https://grass.osgeo.org/grass-stable/manuals/libpython/pygrass_gis.html)

#### Region Management

```python
from grass.pygrass.gis.region import Region

region = Region()

```

## Temporal Framework

```python
import grass.temporal as tgis
```

Full Documentation: [Temporal Framework](https://grass.osgeo.org/grass85/manuals/libpython/temporal.html)

## Message and Error Handling

```python
gs.Message("Hello World")

gs.Warning(_("This is a warning"))
```

```python

try:
    gs.run_command('g.region', raster='elevation')
except gs.CalledModuleError as e:
    gs.error(_("Failed to set region: {e}"))

```

!!! grass-tip "Use Formated Strings"
    <!-- markdownlint-disable-next-line MD046 MD033-->
    When using the `gs.Message` and `gs.error` functions, it is a good idea to
    use formated strings to provide more information about the message or error.
    However, f-strings are not safe to use with the `gs.error` function as they
    <!-- markdownlint-disable-next-line MD046 MD033-->
    can expose the user to injection attacks. Instead, use the `str.format` method.<br>
    <!-- markdownlint-disable-next-line MD046 MD033-->
    :x: `gs.fatal(_(f"Error calculating height above nearest drainage: {e.stderr}"))`<br>
    <!-- markdownlint-disable-next-line MD046 MD033-->
    :white_check_mark: `gs.fatal(_("Error calculating height above nearest
    drainage: %s") % e.stderr)`
