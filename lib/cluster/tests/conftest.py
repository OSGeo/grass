import pytest

import grass.lib.gis as libgis


@pytest.fixture
def libgis_without_session():
    """Let the clustering functions run without a GRASS session

    Most of the I_cluster_*() functions call G_debug(), which reads the
    DEBUG variable out of the session's GISRC file. With no session there
    is no file to read, so the lookup ends in G_fatal_error() and exits
    the process, taking the pytest run down with it. Clustering itself
    never touches a project or mapset, so instead of setting up a session
    for its own sake, GISRC is switched to memory mode: libgis then keeps
    those variables in memory and never looks for the file. Tools that
    legitimately run outside a mapset do the same, see
    general/g.message/main.c and general/g.dirseps/main.c.

    Giving these tests a real session is not an equivalent fix. Reading a
    GISRC file makes libgis cache that session's project for the rest of
    the process, so later tests sharing the process would go looking for
    their own maps in a project that no longer exists.

    The mode is process-global in the C library, so it is restored here.
    """
    previous_mode = libgis.G_get_gisrc_mode()
    libgis.G_set_gisrc_mode(libgis.G_GISRC_MODE_MEMORY)
    try:
        yield
    finally:
        libgis.G_set_gisrc_mode(previous_mode)
