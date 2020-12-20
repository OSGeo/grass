"""
Create objects in GRASS GIS Spatial Database

(C) 2020 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

.. sectionauthor:: Vaclav Petras <wenzeslaus gmail com>
"""


import os
import shutil
import getpass


def create_subproject(database, project, subproject):
    """Creates a subproject in a specified project"""
    project_path = os.path.join(database, project)
    subproject_path = os.path.join(project_path, subproject)
    # create an empty directory
    os.mkdir(subproject_path)
    # copy DEFAULT_WIND file and its permissions from PERMANENT
    # to WIND in the new subproject
    region_path1 = os.path.join(project_path, "PERMANENT", "DEFAULT_WIND")
    region_path2 = os.path.join(project_path, subproject, "WIND")
    shutil.copy(region_path1, region_path2)
    # set permissions to u+rw,go+r (disabled; why?)
    # os.chmod(os.path.join(database,project,subproject,'WIND'), 0644)


def get_default_subproject_name():
    """Returns default name for subproject."""
    try:
        result = getpass.getuser()
        # Raise error if not ascii (not valid subproject name).
        result.encode("ascii")
    except UnicodeEncodeError:
        # Fall back to fixed name.
        result = "user"

    return result
