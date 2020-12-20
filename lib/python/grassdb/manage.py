"""
Managing existing objects in a GRASS GIS Spatial Database

(C) 2020 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

.. sectionauthor:: Vaclav Petras <wenzeslaus gmail com>
"""


import os
import shutil


def delete_subproject(database, project, subproject):
    """Deletes a specified subproject"""
    if subproject == "PERMANENT":
        raise ValueError(
            _("Subproject PERMANENT cannot be deleted (a whole project can be)")
        )
    shutil.rmtree(os.path.join(database, project, subproject))


def delete_project(database, project):
    """Deletes a specified project"""
    shutil.rmtree(os.path.join(database, project))


def delete_grassdb(database):
    """Deletes a specified GRASS database"""
    shutil.rmtree(database)


def rename_subproject(database, project, old_name, new_name):
    """Rename subproject from *old_name* to *new_name*"""
    if old_name == "PERMANENT":
        raise ValueError(_("Subproject PERMANENT cannot be renamed"))
    project_path = os.path.join(database, project)
    os.rename(
        os.path.join(project_path, old_name), os.path.join(project_path, new_name)
    )


def rename_project(database, old_name, new_name):
    """Rename project from *old_name* to *new_name*"""
    os.rename(os.path.join(database, old_name), os.path.join(database, new_name))
