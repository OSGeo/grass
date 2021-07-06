"""Benchmarking for GRASS GIS modules

This subpackage of the grass package is experimental and the API can change anytime.
The API of the package is defined by what is imported in the top-level ``__init__.py``
file of the subpackage.
"""

from .plots import nprocs_plot, num_cells_plot
from .runners import benchmark_nprocs, benchmark_resolutions
