# MODULE:    grass.jupyter.setup
#
# AUTHOR(S): Caitlin Haedrich <caitlin DOT haedrich AT gmail>
#
# PURPOSE:   This module contains functions for launching a GRASS session
#           in Jupyter Notebooks
#
# COPYRIGHT: (C) 2021 Caitlin Haedrich, and by the GRASS Development Team
#
#           This program is free software under the GNU General Public
#           License (>=v2). Read the file COPYING that comes with GRASS
#           for details.

import os
import weakref

import grass.script as gs
import grass.script.setup as gsetup


def _set_notebook_defaults():
    """
    This function sets several GRASS environment variables that are
    important for GRASS to run smoothly in Jupyter.

    It also allows GRASS to overwrite existing maps of the same name.
    """
    # We want functions to raise exceptions and see standard output of
    # the modules in the notebook.
    gs.set_raise_on_error(True)
    gs.set_capture_stderr(True)

    # Allow overwrite of existing maps
    os.environ["GRASS_OVERWRITE"] = "1"


class _JupyterGlobalSession:
    """Represents a global GRASS session for Jupyter Notebooks.

    Do not create objects of this class directly. Use the standalone *init* function
    and an object will be returned to you, e.g.:

    >>> import grass.jupyter as gj
    >>> session = gj.init(...)

    An object ends the session when it is destroyed or when the *finish* method is
    called explicitely.

    Notably, only the mapset is closed, but the libraries and GRASS modules
    remain on path.
    """

    def __init__(self):
        self._finalizer = weakref.finalize(self, gsetup.finish)

    def switch_mapset(self, arg):
        """Switch to a mapset provided as a name or path.

        The *arg* positional-only parameter can be either name of a mapset in the
        current location or a full path to a mapset.

        Raises ValueError if the mapset does not exist or CalledModuleError if
        call to the underlying the g.mapset module fails (e.g., when mapset is
        invalid).
        """
        # The method could be a function, but this is more general (would work even for
        # a non-global session) and users need to keep the reference anyway.
        # pylint: disable=no-self-use
        # Functions needed only here.
        # pylint: disable=import-outside-toplevel
        from grass.grassdb.checks import mapset_exists
        from grass.grassdb.manage import split_mapset_path

        if mapset_exists(arg):
            path, location, mapset = split_mapset_path(arg)
            # TODO: Requires direct session file modification.
            # g.mapset locks the mapset which is not how init and finish behave.
            gs.run_command("g.mapset", dbase=path, location=location, mapset=mapset)
            return
        gisenv = gs.gisenv()
        if mapset_exists(
            path=gisenv["GISDBASE"], location=gisenv["LOCATION_NAME"], mapset=arg
        ):
            gs.run_command("g.mapset", mapset=arg)
            return
        raise ValueError(_("Mapset '{}' does not exist").format(arg))

    def finish(self):
        """Close the session, i.e., close the opened mapset.

        Subsequent calls to GRASS GIS modules will fail because there will be
        no current (opened) mapset anymore.

        The finish procedure is done automatically when process finishes or the object
        is destroyed.
        """
        self._finalizer()


def init(path, location=None, mapset=None, grass_path=None):
    """
    This function initiates a GRASS session and sets GRASS
    environment variables.

    Calling this function returns an object which represents the session.

    >>> import grass.jupyter as gj
    >>> session = gj.init(...)

    When the object is destroyed, the session is ended. Therefore, it is necessary
    to keep a reference to the object as long as the session should remain active.
    In a notebook, this is normally achieved by simply assigning the function return
    value to a variable. If you see ``GISRC - variable not set`` after calling
    a GRASS module, you know you have forgot to assign the result to a variable.

    Since the clean up happens when the object is garbadge-collected, it may or may
    not happen immediately. When you forget to assign the result to a variable, your
    code may still work even if you don't assign it. However, you need to assign it
    to ensure that your code always works.

    The returned object can be used to switch to another mapset:

    >>> session.switch_mapset("mapset_name")

    :param str path: path to grass databases
    :param str location: name of GRASS location
    :param str mapset: name of mapset within location
    """
    # Create a GRASS GIS session.
    gsetup.init(path, location=location, mapset=mapset, grass_path=grass_path)
    # Set GRASS env. variables
    _set_notebook_defaults()
    return _JupyterGlobalSession()
