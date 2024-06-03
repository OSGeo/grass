<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> ba3c0640fa (libpython: Support benchmarks of non-parallel runs better (#1733))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
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

<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
>>>>>>> da7f79c3f9 (libpython: Save and load benchmark results (#1711))
=======
>>>>>>> ba3c0640fa (libpython: Support benchmarks of non-parallel runs better (#1733))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
"""Benchmarking for GRASS GIS modules

This subpackage of the grass package is experimental and the API can change anytime.
The API of the package is defined by what is imported in the top-level ``__init__.py``
file of the subpackage.
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> ba3c0640fa (libpython: Support benchmarks of non-parallel runs better (#1733))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main

The functions in the Python API raise exceptions, although calls of other functions from
the grass package may call grass.script.fatal and exit
(see :func:`grass.script.core.set_raise_on_error` for changing the behavior).
This applies to the CLI interface of this subpackage too except that raised usage
exceptions originating in the CLI code result in *sys.exit* with an error message, not
traceback. Messages and other user-visible texts in this package are not translatable.
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
>>>>>>> da7f79c3f9 (libpython: Save and load benchmark results (#1711))
=======
>>>>>>> ba3c0640fa (libpython: Support benchmarks of non-parallel runs better (#1733))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
"""

from .plots import nprocs_plot, num_cells_plot
from .results import (
    join_results,
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    join_results_from_files,
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    join_results_from_files,
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
    join_results_from_files,
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
    join_results_from_files,
=======
>>>>>>> osgeo-main
>>>>>>> da7f79c3f9 (libpython: Save and load benchmark results (#1711))
=======
    join_results_from_files,
>>>>>>> ba3c0640fa (libpython: Support benchmarks of non-parallel runs better (#1733))
=======
    join_results_from_files,
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    load_results,
    load_results_from_file,
    save_results,
    save_results_to_file,
)
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
from .runners import benchmark_nprocs, benchmark_resolutions, benchmark_single
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
from .runners import benchmark_nprocs, benchmark_resolutions, benchmark_single
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
from .runners import benchmark_nprocs, benchmark_resolutions, benchmark_single
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
from .runners import benchmark_nprocs, benchmark_resolutions, benchmark_single
=======
>>>>>>> osgeo-main
from .runners import benchmark_nprocs, benchmark_resolutions
>>>>>>> da7f79c3f9 (libpython: Save and load benchmark results (#1711))
=======
from .runners import benchmark_nprocs, benchmark_resolutions, benchmark_single
>>>>>>> ba3c0640fa (libpython: Support benchmarks of non-parallel runs better (#1733))
=======
from .runners import benchmark_nprocs, benchmark_resolutions, benchmark_single
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
