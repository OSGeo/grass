# -*- coding: utf-8 -*-
"""!@package grass.gunittest

@brief GRASS Python testing framework module for running from command line

Copyright (C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS GIS
for details.

@author Vaclav Petras
@author Soeren Gebbert

Initial version of `gunittest` was created during Google Summer of Code 2014
by Vaclav Petras as a student and Soeren Gebbert as a mentor.
"""

from __future__ import print_function

try:
    from .case import TestCase
    from .main import test
except ImportError, e:
    print('WARNING: Cannot import ({e.message}).\n'
          'Ignoring the failed import because it does not harm if you need'
          ' something different'
          ' from gunittest. Probably the environment is not set properly'
          ' (e.g. dynamic libraries are not available and ctypes-based modules'
          ' cannot work).'.format(e=e))
    # we need to ignore import errors for the cases when we just need
    # gunittest for reports and ctypes are not available (or the environment
    # is not set properly)
    # .main probably does not need to be checked but it imports a lot of
    # things, so it might be hard to keep track in the future
    # .case imports PyGRASS which imports ctypes modules in its __init__.py
