---
authors:
    - Corey T. White
    - GRASS Development Team
---

# Command Line Introduction

## Interactive Shell

```text
Starting GRASS...
          __________  ___   __________
         / ____/ __ \/   | / ___/ ___/
        / / __/ /_/ / /| | \__ \\_  \
       / /_/ / _, _/ ___ |___/ /__/ /
       \____/_/ |_/_/  |_/____/____/

```

The GRASS command line interface allows you to start a GRASS session to
run GRASS commands, execute scripts, or open the GUI.

Here we create a new project for the NAD83(HARN)/North Carolina coordinate
reference system (EPSG:3358) and start a GRASS session:

```sh
grass -c EPSG:3358 {project directory} --text
```

We can now execute GRASS tools as commands in a new shell. For example, we can
import an elevation raster and calculate its
[univariate statistics](r.univar.md).

```sh
r.import input=elevation.tif output=elevation resample=bilinear
```

Let's make sure the region is set to the elevation raster.

```sh
g.region raster=elevation -p

projection: 99 (Lambert Conformal Conic)
zone:       0
datum:      nad83
ellipsoid:  a=6378137 es=0.006694380022900787
north:      228500
south:      215000
west:       630000
east:       645000
nsres:      10
ewres:      10
rows:       1350
cols:       1500
cells:      2025000

```

Now we can calculate the univariate statistics of the elevation raster.

```sh
r.univar elevation

total null and non-null cells: 2025000
total null cells: 0

Of the non-null cells:
----------------------
n: 2025000
minimum: 55.5788
maximum: 156.33
range: 100.751
mean: 110.375
mean of absolute values: 110.375
standard deviation: 20.3153
variance: 412.712
variation coefficient: 18.4057 %
sum: 223510266.558102

```

## Command Execution

GRASS commands can be executed directly in the terminal. Here we execute the
[r.slope.aspect](r.slope.aspect.md) tool on an elevation raster from our
project.

```sh
grass {project directory} --exec \
    r.slope.aspect elevation=elevation slope=slope aspect=aspect
```

## Scripting

The *grass* command can also execute scripts. Here we create a Bash script,
`export_slope.sh`,  to export the slope map as a PNG.

```bash
#!/bin/bash
g.region raster=elevation
r.slope.aspect elevation=elevation slope=slope aspect=aspect
r.colors map=slope color=sepia
r.out.png input=slope output=slope -w
```

```sh
grass {project directory} --exec bash export_slope.sh
```

Python scripts can also be executed by the *grass* command when a Python
interpreter is provided. Here we create a Python script, `export_aspect.py`,
to export the aspect map as a PNG.

```python
import grass.script as gs

gs.run_command('g.region', raster='elevation')
gs.run_command('r.slope.aspect', elevation='elevation', slope='slope', aspect='aspect')
gs.run_command('r.colors', map='aspect', color='aspect')
gs.run_command('r.out.png', input='aspect', output='aspect', overwrite=True)
```

Now execute the Python script.

```sh
grass {project directory} --exec python export_aspect.py
```

Scripts can also run from the interactive shell providing adding to
the flexibility of the command line interface.
