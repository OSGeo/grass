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

from .case import TestCase
from .main import test
