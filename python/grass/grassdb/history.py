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

from grass.script import gisenv


def get_current_mapset_gui_history_path():
    """Return path to the current mapset history file."""
    env = gisenv()
    return os.path.join(
        env["GISDBASE"], env["LOCATION_NAME"], env["MAPSET"], ".wxgui_history"
    )


def create_history_file(history_path):
    """Set up a new GUI history file."""
    try:
        fileHistory = open(
            history_path,
            encoding="utf-8",
            mode="w",
        )
    except OSError as e:
        raise OSError(_("Unable to create history file {}").format(history_path)) from e
    finally:
        fileHistory.close()


def read_history(history_path):
    """Get list of commands from history file."""
    hist = list()
    try:
        fileHistory = open(
            history_path,
            encoding="utf-8",
            mode="r",
            errors="replace",
        )
    except OSError as e:
        raise OSError(
            _("Unable to read commands from history file {}").format(history_path)
        ) from e
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
        history_path = get_current_mapset_gui_history_path()
    try:
        if os.path.exists(history_path):
            fileHistory = open(history_path, encoding="utf-8", mode="a")
        else:
            fileHistory = open(history_path, encoding="utf-8", mode="w")
        fileHistory.write(command + "\n")
    except OSError as e:
        raise OSError(_("Unable to update history file {}").format(history_path)) from e
    finally:
        fileHistory.close()


def copy_history(target_path, history_path):
    """Copy history file to the target location.
    Returns True if file is successfully copied."""
    try:
        shutil.copyfile(history_path, target_path)
    except OSError as e:
        raise OSError(
            _("Unable to copy file {} to {}").format(history_path, target_path)
        ) from e
    return True
