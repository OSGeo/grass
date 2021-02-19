# -*- coding: utf-8 -*-
"""Utilities related to GRASS GIS for GRASS Python testing framework

Copyright (C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS GIS
for details.

:authors: Vaclav Petras
"""

from grass.script.core import start_command, PIPE
from grass.script.utils import decode

from .gmodules import call_module
from .checkers import text_to_keyvalue


def get_current_mapset():
    """Get curret mapset name as a string"""
    return call_module('g.mapset', flags='p').strip()

def is_map_in_mapset(name, type, mapset=None):
    """Check is map is present in the mapset (current mapset by default)

    This function is different from what we would expect in GRASS
    because it cares only about specific mapset, the current one by default,
    and it does not care that the map is accessible in other mapset.

    :param name: name of the map
    :param type: data type ('raster', 'raster3d', and 'vector')
    """
    if not mapset:
        mapset = get_current_mapset()

    # change type to element used by find file
    # otherwise, we are not checking the input,
    # so anything accepted by g.findfile will work but this can change in the
    # future (the documentation is clear about what's legal)
    # supporting both short and full names
    if type == 'rast' or  type == 'raster':
        type = 'cell'
    elif type == 'rast3d' or type == 'raster3d':
        type = 'grid3'
    elif type == 'vect':
        type = 'vector'
    # g.findfile returns non-zero when file was not found
    # se we ignore return code and just focus on stdout
    process = start_command('g.findfile', flags='n',
                            element=type, file=name, mapset=mapset,
                            stdout=PIPE, stderr=PIPE)
    output, errors = process.communicate()
    info = text_to_keyvalue(decode(output), sep='=')
    # file is the key questioned in grass.script.core find_file()
    # return code should be equivalent to checking the output
    if info['file']:
        return True
    else:
        return False
