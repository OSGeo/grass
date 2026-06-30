import os

from .ctypes_loader import add_library_search_dirs

# Register $GISBASE/lib with the ctypes loader so wrapper modules resolve GRASS
# shared libraries by absolute path, skipping the expensive
# ctypes.util.find_library() fallback. No-op when GISBASE is unset.
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
