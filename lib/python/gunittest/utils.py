# -*- coding: utf-8 -*-
"""!@package grass.gunittest.utils

@brief GRASS Python testing framework utilities (general and test-specific)

Copyright (C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS GIS
for details.

@author Vaclav Petras
"""

import os
import sys
import shutil
import errno


def ensure_dir(directory):
    """Create all directories in the given path if needed."""
    if not os.path.exists(directory):
        os.makedirs(directory)


def silent_rmtree(filename):
    """Remove the file but do nothing if file does not exist."""
    try:
        shutil.rmtree(filename)
    except OSError as e:
        # errno.ENOENT is "No such file or directory"
        # re-raise if a different error occured
        if e.errno != errno.ENOENT:
            raise


def do_doctest_gettext_workaround():
    """Setups environment for doing a doctest with gettext usage.

    When using gettext with dynamically defined underscore function
    (``_("For translation")``), doctest does not work properly. One option is
    to use `import as` instead of dynamically defined underscore function but
    this would require change all modules which are used by tested module.
    This should be considered for the future. The second option is to define
    dummy underscore function and one other function which creates the right
    environment to satisfy all. This is done by this function.
    """
    def new_displayhook(string):
        """A replacement for default `sys.displayhook`"""
        if string is not None:
            sys.stdout.write("%r\n" % (string,))

    def new_translator(string):
        """A fake gettext underscore function."""
        return string

    sys.displayhook = new_displayhook

    import __builtin__
    __builtin__._ = new_translator
