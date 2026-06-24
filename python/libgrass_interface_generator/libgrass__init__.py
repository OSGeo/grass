import os

from .ctypes_loader import add_library_search_dirs

# Register $GISBASE/lib (where the GRASS shared libraries are installed) with
# the ctypes loader before any wrapper module loads its library. The loader
# searches these directories first, so it finds the libraries by absolute path
# and never reaches its ctypes.util.find_library() fallback. That fallback is
# expensive on Linux, where find_library() spawns the toolchain (ldconfig, gcc,
# ld, objdump) once per library; for the chain pulled in by grass.temporal that
# was about 90 subprocesses and the bulk of the import time. The libraries are
# still resolvable through the runtime linker path, so this is a startup
# optimization, not a correctness fix; outside a GRASS session GISBASE may be
# unset, in which case the loader keeps its original behavior.
_gisbase = os.environ.get("GISBASE")
if _gisbase:
    add_library_search_dirs([os.path.join(_gisbase, "lib")])

__all__ = [
    'arraystats',
    'cluster',
    'date',
    'dbmi',
    'display',
    'gis',
    'gmath',
    'imagery',
    'nviz',
    'ogsf',
    'proj',
    'raster3d',
    'raster',
    'rtree',
    'stats',
    'vector',
    'vedit'
]
