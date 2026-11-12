import os

from .ctypes_loader import add_library_search_dirs, load_library

# The name under which the wrapper modules load the core library, set when
# this file is installed.
_CORE_LIBRARY_NAME = "@GRASS_CORE_LIBRARY_NAME@"


def _core_library_path(lib_path):
    """Get the path of the core library in a directory, or None

    The file names are those the wrapper modules look for, so a directory
    with the core library of another GRASS version does not match.
    """
    for name_format in load_library.name_formats:
        path = os.path.join(lib_path, name_format % _CORE_LIBRARY_NAME)
        if os.path.isfile(path):
            return path
    return None


def _find_lib_path():
    """Find the directory with the GRASS libraries this package belongs to.

    The directory is derived from the location of this file, which also
    guarantees that the libraries match the wrapper modules generated for
    them. The package is at <install path>/etc/python/grass/lib with the
    libraries in <install path>/lib, or, in an FHS layout, in site-packages
    under the library directory, e.g., <prefix>/lib/python3.12/site-packages.
    GISBASE is used as a fallback for other layouts.
    """
    path = os.path.dirname(os.path.realpath(__file__))
    # Four levels up from grass/lib (through lib, grass, python, and etc) is
    # the installation in the traditional layout, and (through lib, grass,
    # site-packages, and python3.X) the library directory in the FHS layout.
    for _ in range(4):
        path = os.path.dirname(path)
    candidates = [os.path.join(path, "lib"), path]
    gisbase = os.environ.get("GISBASE")
    if gisbase:
        candidates.append(os.path.join(gisbase, "lib"))
    for candidate in candidates:
        if _core_library_path(candidate):
            return candidate
    return None


_lib_path = _find_lib_path()
if _lib_path:
    # Registering the directory makes the wrapper modules resolve the
    # libraries by absolute path, skipping the expensive
    # ctypes.util.find_library() fallback.
    add_library_search_dirs([_lib_path])

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
