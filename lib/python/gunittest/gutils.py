# -*- coding: utf-8 -*-
"""!@package grass.gunittest.gutils

@brief Utilities related to GRASS GIS for GRASS Python testing framework

Copyright (C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS GIS
for details.

@author Vaclav Petras
"""
from .gmodules import call_module


def get_current_mapset():
    """Get curret mapset name as a string"""
    return call_module('g.mapset', flags='p').strip()
