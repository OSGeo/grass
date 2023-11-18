"""
Managing existing history file included within current mapset

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


def get_current_mapset_history_path():
    """Returns path to the current mapset history file
    or None if current mapset has no history."""
    env = gisenv()
    return os.path.join(
        env["GISDBASE"],
        env["LOCATION_NAME"],
        env["MAPSET"],
        ".wxgui_history",
    )


def read_history():
    """Get list of commands from history file"""
    hist = list()
    try:
        fileHistory = codecs.open(
            get_current_mapset_history_path(),
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


def update_history(command):
    """Update history file

    :param command: the command given as a string
    """
    try:
        fileHistory = codecs.open(
            get_current_mapset_history_path(), encoding="utf-8", mode="a"
        )
    except IOError as e:
        GError(_("Unable to write file {}'.\n\nDetails: {}").format(fileHistory, e))
        return

    try:
        fileHistory.write(command + os.linesep)
    finally:
        fileHistory.close()


def copy_history(targetFile):
    """Copy history file to the target location.
    Returns True if file is successfully copied."""
    historyFile = get_current_mapset_history_path()
    try:
        shutil.copyfile(historyFile, targetFile)
    except (IOError, OSError) as e:
        GError(
            _("Unable to copy file {} to {}'.\n\nDetails: {}").format(
                historyFile, targetFile, e
            )
        )
        return False
    return True
