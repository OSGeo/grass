The current command line rendering mechanism is direct rendering into a
file. The driver is selected by setting the `GRASS_RENDER_IMMEDIATE`
variable or by running *[d.mon](d.mon.html)* module.

**List of available display drivers:**

-   [Cairo driver](cairodriver.html)
-   [PNG driver](pngdriver.html)
-   [PS driver (Postscript)](psdriver.html)
-   [HTMLMAP driver](htmldriver.html)

## NOTES

### GRASS_RENDER_COMMAND

If environmental variable GRASS_RENDER_COMMAND is defined, rendering is
redirected by display library to the given external command defined by
this variable. Currently only Python scrips are supported.

Lets start with simple example of Python script called *render.py*:

```
#!/usr/bin/env python3

import os
import sys

import grass.script as gs
from grass.script import task as gtask

os.environ['GRASS_RENDER_IMMEDIATE'] = 'default'
os.environ['GRASS_RENDER_FILE'] = 'output.png'

cmd, dcmd = gtask.cmdstring_to_tuple(sys.argv[1])

gs.run_command('d.text', text="Test of GRASS_RENDER_COMMAND redirection")

os.environ['GRASS_RENDER_FILE_READ'] = 'TRUE'
gs.run_command(cmd, **dcmd)
```

After defining GRASS_RENDER_COMMAND variable (example for Bash):

```
export GRASS_RENDER_COMMAND=render.py
```

Display GRASS modules like *[d.rast](d.rast.html)* or
*[d.vect](d.vect.html)* will be executed by *render.py* program. For
example the command

```
d.vect roadsmajor
```

produces output PNG file *output.png* which will contain rendered
features from vector map *roadsmajor* and sample text *\"Test of
GRASS_RENDER_COMMAND redirection\"*.

## SEE ALSO

*[d.mon](d.mon.html),
[variables](variables.html#list-of-selected-grass-environment-variables-for-rendering)*
