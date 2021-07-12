<<<<<<< HEAD
# MODULE:    grass.benchmark
#
# AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>
#
# PURPOSE:   Benchmarking for GRASS GIS modules
#
# COPYRIGHT: (C) 2021 Vaclav Petras, and by the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.

=======
>>>>>>> da7f79c3f9 (libpython: Save and load benchmark results (#1711))
"""Benchmarking for GRASS GIS modules

This subpackage of the grass package is experimental and the API can change anytime.
The API of the package is defined by what is imported in the top-level ``__init__.py``
file of the subpackage.
<<<<<<< HEAD

The functions in the Python API raise exceptions, although calls of other functions from
the grass package may call grass.script.fatal and exit
(see :func:`grass.script.core.set_raise_on_error` for changing the behavior).
This applies to the CLI interface of this subpackage too except that raised usage
exceptions originating in the CLI code result in *sys.exit* with an error message, not
traceback. Messages and other user-visible texts in this package are not translatable.
=======
>>>>>>> da7f79c3f9 (libpython: Save and load benchmark results (#1711))
"""

from .plots import nprocs_plot, num_cells_plot
from .results import (
    join_results,
<<<<<<< HEAD
    join_results_from_files,
=======
>>>>>>> da7f79c3f9 (libpython: Save and load benchmark results (#1711))
    load_results,
    load_results_from_file,
    save_results,
    save_results_to_file,
)
<<<<<<< HEAD
from .runners import benchmark_nprocs, benchmark_resolutions, benchmark_single
=======
from .runners import benchmark_nprocs, benchmark_resolutions
>>>>>>> da7f79c3f9 (libpython: Save and load benchmark results (#1711))
