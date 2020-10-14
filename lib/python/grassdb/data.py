"""
Manipulate data in mapsets in GRASS GIS Spatial Database

(C) 2020 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

.. sectionauthor:: Vaclav Petras <wenzeslaus gmail com>
"""

import grass.script as gs


def map_exists(name, element, mapset=None, env=None):
    """Check is map is present in the mapset given in the environment

    :param name: Name of the map
    :param element: Data type ('raster', 'raster_3d', and 'vector')
    :param env: Environment created by function grass.script.create_environment
    :param mapset: Mapset name, "." for current mapset only,
                   None for all mapsets in the search path
    """
    # change type to element used by find file
    if element == "raster":
        element = "cell"
    elif element == "raster_3d":
        element = "grid3"
    # g.findfile returns non-zero when file was not found
    # se we ignore return code and just focus on stdout
    process = gs.start_command(
        "g.findfile",
        flags="n",
        element=element,
        file=name,
        mapset=mapset,
        stdout=gs.PIPE,
        stderr=gs.PIPE,
        env=env,
    )
    output, unused_errors = process.communicate()
    info = gs.parse_key_val(output, sep="=")
    # file is the key questioned in grass.script.core find_file()
    # return code should be equivalent to checking the output
    if info["file"]:
        return True
    return False
