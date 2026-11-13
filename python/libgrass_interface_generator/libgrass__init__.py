import ctypes
import os

from .ctypes_loader import add_library_search_dirs, load_library

# The name under which the wrapper modules load the core library, set by the
# build.
_CORE_LIBRARY_NAME = "@GRASS_CORE_LIBRARY_NAME@"


def _core_library_path(lib_path):
    """Get the path of the core library in a directory, or None

    The file names are those under which the wrapper modules load the core
    library.
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


# Most GRASS libraries need libgrass_gis and libgrass_gis needs
# libgrass_datetime, so these two are loaded first. A library loaded before
# the GRASS libraries it needs makes the dynamic linker find them through the
# process-startup search path, which may point to another GRASS installation.
# A second copy of libgrass_gis breaks the libraries using it because they
# rely on the session state it holds.
_BASE_LIBRARY_NAMES = ("datetime", "gis")


def _preload_libraries(lib_path):
    """Load the GRASS libraries in a directory into the process by full path.

    Libraries can be loaded only after the libraries they depend on, so
    loading is repeated until it makes no progress. Windows DLLs are not
    matched because Windows finds dependent DLLs without this loading.

    Returns a list of (path, error) pairs for libraries which failed to load.
    """
    # Versioned files and the unversioned symlinks pointing to them are both
    # listed, so the real paths are collected to load each file once.
    libraries = {
        os.path.realpath(os.path.join(lib_path, name))
        for name in os.listdir(lib_path)
        if name.startswith("libgrass_")
        and (name.endswith((".so", ".dylib")) or ".so." in name)
    }

    def load_order(library):
        name = os.path.basename(library).removeprefix("libgrass_").partition(".")[0]
        if name in _BASE_LIBRARY_NAMES:
            return (_BASE_LIBRARY_NAMES.index(name), name)
        return (len(_BASE_LIBRARY_NAMES), name)

    remaining = sorted(libraries, key=load_order)
    while remaining:
        failed = []
        errors = []
        for library in remaining:
            try:
                ctypes.CDLL(library)
            except OSError as error:
                failed.append(library)
                errors.append((library, error))
        if len(failed) == len(remaining):
            return errors
        remaining = failed
    return []


def _load_libraries_if_needed(lib_path):
    """Load the GRASS libraries unless they already find each other.

    Loading the libraries into the process by their full paths makes them
    find each other even when the process was started without the library
    search path variable and the search path recorded in the libraries at
    build time does not point to them, e.g., for an installation moved
    away from the prefix it was configured with.
    """
    core_library = _core_library_path(lib_path)
    if not core_library:
        return
    try:
        # Nothing more is needed when the core library finds the GRASS
        # libraries it depends on by itself.
        ctypes.CDLL(core_library)
    except OSError:
        pass
    else:
        return
    for library, error in _preload_libraries(lib_path):
        if library == os.path.realpath(core_library):
            msg = (
                f"Cannot load the GRASS core library {library}: {error}. Check"
                " that the GRASS installation and its dependencies are complete."
            )
            raise ImportError(msg)


_lib_path = _find_lib_path()
if _lib_path:
    # Registering the directory makes the wrapper modules resolve the
    # libraries by absolute path, skipping the expensive
    # ctypes.util.find_library() fallback.
    add_library_search_dirs([_lib_path])
    _load_libraries_if_needed(_lib_path)

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
