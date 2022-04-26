# MODULE:    grass.jupyter
#
# AUTHOR(S): Caitlin Haedrich <caitlin DOT haedrich AT gmail>
#            Vaclav Petras <wenzeslaus gmail com>
#            Anna Petrasova <kratochanna gmail com>
#
# PURPOSE:   Display classes and setup functions for running GRASS GIS
#            in Jupyter Notebooks
#
# COPYRIGHT: (C) 2021-2022 Caitlin Haedrich, and by the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.

"""Display classes and setup functions for running GRASS GIS in Jupyter Notebooks

The *grass.jupyter* subpackage improves the integration of GRASS GIS and Jupyter
Notebooks. The original version was written as part of Google Summer of Code in 2021
and experimental version was included in GRASS GIS 8.0. Since then, much more
development happened adding better session handling and rendering of additional data
types.

For standard usage, simply import the top level package with a convenient alias, e.g.,::

>>> import grass.jupyter as gj

The objects in submodules and names of submodules may change in the future.

.. note::
    To import the package, you need to tell Python where the GRASS GIS Python package
    is. Please, refer to example notebooks for an example of the full workflow.

.. note::
    Although most of the functionality is general, the defaults, resource management,
    and other behavior assumes usage in an interactive notebook, so using the
    functionality in other contexts (e.g. a script) may result in unexpected behavior.
    Consult the documentation or mailing list if in doubt. Suggest generalized
    functionality using issues and pull requests.

.. versionadded:: 8.2
"""

from .interactivemap import Raster, Vector, InteractiveMap
from .map import Map
from .map3d import Map3D
from .setup import init
from .timeseriesmap import TimeSeriesMap
