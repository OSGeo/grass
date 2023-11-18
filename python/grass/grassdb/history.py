"""
Managing existing history files included within mapset

(C) 2023 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

.. sectionauthor:: Linda Karlovska (Kladivova) linda.karlovska@seznam.cz
"""

import os
import shutil
import codecs

from grass.script import gisenv
from core.gcmd import GError


def get_mapset_history_path(mapset_path, history_file_name):
    """Returns path to the mapset history file or None if mapset has no history."""
    history_file = os.path.join(mapset_path, history_file_name)
    return history_file if os.path.exists(history_file) else None


def get_current_mapset_history_path():
    """Returns path to the current mapset history file
    or None if current mapset has no history."""
    env = gisenv()
    mapsetPath = os.path.join(env["GISDBASE"], env["LOCATION_NAME"], env["MAPSET"])
    return get_mapset_history_path(mapsetPath, ".wxgui_history")


def read_history(history_path):
    """Get list of commands from history file."""
    hist = list()
    try:
        fileHistory = codecs.open(
            history_path,
            encoding="utf-8",
            mode="r",
            errors="replace",
        )
    except IOError:
        return hist

    try:
        for line in fileHistory.readlines():
            hist.append(line.replace("\n", ""))
    finally:
        fileHistory.close()

    return hist


def update_history(command, history_path=None):
    """Update history file.

    :param command: the command given as a string
    """
    if not history_path:
        history_path = get_current_mapset_history_path()
    try:
        fileHistory = codecs.open(history_path, encoding="utf-8", mode="a")
    except IOError as e:
        GError(_("Unable to write file {}'.\n\nDetails: {}").format(fileHistory, e))
        return

    try:
        fileHistory.write(command + os.linesep)
    finally:
        fileHistory.close()


def copy_history(target_path, history_path):
    """Copy history file to the target location.
    Returns True if file is successfully copied."""
    try:
        shutil.copyfile(history_path, target_path)
    except (IOError, OSError) as e:
        GError(
            _("Unable to copy file {} to {}'.\n\nDetails: {}").format(
                history_path, target_path, e
            )
        )
        return False
    return True
