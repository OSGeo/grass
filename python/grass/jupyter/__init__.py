# MODULE:    grass.jupyter
#
# AUTHOR(S): Caitlin Haedrich <caitlin DOT haedrich AT gmail>
#
# PURPOSE:   Display classes and setup functions for running GRASS GIS
#            in Jupyter Notebooks
#
# COPYRIGHT: (C) 2021 Caitlin Haedrich, and by the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.

"""Display classes and setup functions for running GRASS GIS in Jupyter Notebooks

This subpackage of the grass package is experimental and under development. The API
can change at anytime.

The grass.jupyter subpackage improves the integration of GRASS GIS and Jupyter
Notebooks. The original version was written as part of Google Summer of Code in 2021.
For more information, visit https://trac.osgeo.org/grass/wiki/GSoC/2021/JupyterAndGRASS
"""

from .setup import *
from .interact_display import *
from .display import *
from .utils import *
