"""
Managing existing history files included within mapset

(C) 2023 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

.. sectionauthor:: Linda Karlovska (Kladivova) linda.karlovska@seznam.cz
"""

import json
import shutil
from enum import Enum
from pathlib import Path

from datetime import datetime
import grass.script as gs


class Status(Enum):
    """Enum representing a set of status constants
    that are used to represent various states or command outcomes."""

    ABORTED = "aborted"
    FAILED = "failed"
    RUNNING = "running"
    SUCCESS = "success"
    UNKNOWN = "unknown"


def get_current_mapset_gui_history_path():
    """Return path to the current mapset history file.
    This function does not ensure that the file exists.
    """
    env = gs.gisenv()
    base_path = Path(env["GISDBASE"]) / env["LOCATION_NAME"] / env["MAPSET"]
    history_filename = ".wxgui_history"

    txt_path = base_path / history_filename
    json_path = base_path / (history_filename + ".json")

    # Return path txt only if it exists and json does not
    return txt_path if txt_path.exists() and not json_path.exists() else json_path


def get_history_file_extension(history_path):
    """Get extension of the history file.
    The standard long-term format is plain text.
    Newly it is stored as JSON-formatted.

    :param str history_path: path to the history log file
    :return str extension: None (plain text) or .json
    """
    file_path = Path(history_path)
    return file_path.suffix


def ensure_history_file(history_path):
    """Set up a new GUI history file if it doesn't exist.

    :param str history_path: path to the history file
    """
    if not history_path.exists():
        try:
            with open(history_path, encoding="utf-8", mode="w"):
                pass
        except OSError as e:
            raise OSError(
                _("Unable to create history file {}").format(history_path)
            ) from e


def _read_from_plain_text(history_path):
    """Read content of plain text history file.

    :param str history_path: path to the history log file
    :return content_list: list of dictionaries
    with 'command' and 'command_info' keys
    'command_info' is always empty since plain text history file
    stores only executed commands."""
    content_list = []
    try:
        with open(history_path, encoding="utf-8", errors="replace") as file_history:
            content_list = [
                {"command": line.strip(), "command_info": None} for line in file_history
            ]
    except OSError as e:
        raise OSError(
            _("Unable to read from plain text history file {}").format(history_path)
        ) from e
    return content_list


def _read_from_JSON(history_path):
    """Read content of JSON history file.

    :param str history_path: path to the history log file
    :return content_list: list of dictionaries
    with 'command' and 'command_info' keys
    """
    content_list = []
    try:
        content = Path(history_path).read_text(encoding="utf-8", errors="replace")
        if content:
            try:
                history_entries = json.loads(content)
            except ValueError as ve:
                raise ValueError(
                    _("Error decoding content of JSON history file {}").format(
                        history_path
                    )
                ) from ve
            # Process the content as a list of dictionaries
            content_list = [
                {
                    "command": entry["command"],
                    "command_info": entry["command_info"],
                }
                for entry in history_entries
            ]
    except OSError as e:
        raise OSError(
            _("Unable to read from JSON history file {}").format(history_path)
        ) from e
    return content_list


def read(history_path):
    """Read the content of the history file.

    :param str history_path: path to the history log file
    :return content_list: list of dictionaries
    with 'command' and 'command_info' keys
    """
    if get_history_file_extension(history_path) == ".json":
        return _read_from_JSON(history_path)
    return _read_from_plain_text(history_path)


def filter(json_data, command, timestamp):
    """
    Filter JSON history file based on provided command and the time of command launch.

    :param json_data: List of dictionaries representing JSON entries
    :param command: First filtering argument representing command as string
    :param timestamp: Second filtering argument representing the time of command launch
    :return: Index of entry matching the filter criteria.
    """
    for index, entry in enumerate(json_data):
        if entry["command_info"]:
            if (
                entry["command"] == command
                and entry["command_info"]["timestamp"] == timestamp
            ):
                return index
    return None


def _remove_entry_from_plain_text(history_path, index: int):
    """Remove entry from plain text history file.

    :param str history_path: path to the history log file
    :param int index: index of the command to be removed
    """
    try:
        with open(history_path, encoding="utf-8", mode="r+") as file_history:
            lines = file_history.readlines()
            file_history.seek(0)
            file_history.truncate()
            for number, line in enumerate(lines):
                if number != index:
                    file_history.write(line)
    except OSError as e:
        raise OSError(
            _("Unable to remove entry from plain text history file {}").format(
                history_path
            )
        ) from e


def _remove_entry_from_JSON(history_path, index: int):
    """Remove entry from JSON history file.

    :param str history_path: path to the history log file
    :param int index: index of the command to be removed
    """
    try:
        with open(history_path, encoding="utf-8", mode="r+") as file_history:
            try:
                history_entries = json.load(file_history)
                # Check if the index is valid
                if 0 <= index < len(history_entries):
                    # Remove the entry at the specified index
                    del history_entries[index]
                # Write the modified history back to the file
                file_history.seek(0)
                file_history.truncate()
                json.dump(history_entries, file_history, indent=2)
            except ValueError as ve:
                raise ValueError(
                    _("Error decoding content of JSON history file {}").format(
                        history_path
                    )
                ) from ve
    except OSError as e:
        raise OSError(
            _("Unable to remove entry from JSON history file {}").format(history_path)
        ) from e


def remove_entry(history_path, index: int):
    """Remove entry from history file.

    :param str history_path: path to the history log file
    :param int index: index of the command to be removed
    """
    if get_history_file_extension(history_path) == ".json":
        _remove_entry_from_JSON(history_path, index)
    else:
        _remove_entry_from_plain_text(history_path, index)


def convert_plain_text_to_JSON(history_path):
    """Convert plain text history log to JSON format.

    :param str history_path: path to the history log file
    """
    try:
        lines = _read_from_plain_text(history_path)

        # Extract file path and name without extension
        file_path = history_path.parent
        file_name_no_ext = history_path.stem

        # JSON file path
        json_path = file_path / (file_name_no_ext + ".json")

        # Write JSON data to a new file
        with open(json_path, encoding="utf-8", mode="w") as json_outfile:
            json.dump(lines, json_outfile, indent=2)

    except OSError as e:
        raise OSError(
            _("Unable to convert plain text history file {} to JSON format").format(
                history_path
            )
        ) from e


def get_initial_command_info(env_run):
    """Get information about the launched command.

    :param env_run: environment needed for processing grass command
    :return cmd_info dict: initial information about the launched command
    """
    # Execution timestamp in ISO 8601 format
    exec_time = datetime.now().isoformat()

    # 2D raster mask presence
    mask2d_status = gs.parse_command("r.mask.status", format="json", env=env_run)

    # 3D raster mask presence
    env = gs.gisenv(env_run)
    mapset_path = Path(env["GISDBASE"]) / env["LOCATION_NAME"] / env["MAPSET"]
    mask3d_present = (mapset_path / "grid3" / "RASTER3D_MASK").exists()
    mask3d_name = f"RASTER3D_MASK@{env['MAPSET']}"

    # Computational region settings
    region_settings = gs.region(env=env_run)

    # Finalize the command info dictionary
    return {
        "timestamp": exec_time,
        "mask2d": mask2d_status["present"],
        "mask2d_name": mask2d_status["name"],
        "mask3d": mask3d_present,
        "mask3d_name": mask3d_name,
        "region": region_settings,
        "status": Status.RUNNING.value,
    }


def _add_entry_to_JSON(history_path, entry):
    """Add entry to JSON history file.

    :param str history_path: path to the history log file
    :param dict entry: entry consisting of 'command' and 'command_info' keys
    """
    try:
        with open(history_path, encoding="utf-8") as file_history:
            existing_data = json.load(file_history)
    except (OSError, ValueError):
        existing_data = []

    existing_data.append(entry)
    try:
        with open(history_path, encoding="utf-8", mode="w") as file_history:
            json.dump(existing_data, file_history, indent=2)
    except ValueError as ve:
        raise ValueError(
            _("Error decoding content of JSON history file {}").format(history_path)
        ) from ve
    except OSError as e:
        raise OSError(
            _("Unable to add entry to JSON history file {}").format(history_path)
        ) from e


def add_entry(history_path, entry):
    """Add entry to history file.

    :param str history_path: path to the history log file
    :param dict entry: entry consisting of 'command' and 'command_info' keys
    """
    if get_history_file_extension(history_path) != ".json":
        msg = "Adding entries is supported only for JSON format."
        raise ValueError(msg)
    _add_entry_to_JSON(history_path, entry)


def _update_entry_in_JSON(history_path, command_info, index=None):
    """Update entry in JSON history file. If index is None it updates a last entry.

    :param str history_path: path to the history log file
    :param dict command_info: command info entry for update
    :param int|None index: index of the command to be updated
    """
    try:
        with open(history_path, encoding="utf-8", mode="r+") as file_history:
            try:
                history_entries = json.load(file_history)
                if index is None:
                    # Find index of last entry
                    index = len(history_entries) - 1
                # Check if the index is valid
                if 0 <= index < len(history_entries):
                    # Update entry
                    history_entries[index]["command_info"].update(command_info)
                    # Write the modified history back to the file
                    file_history.seek(0)
                    file_history.truncate()
                    json.dump(history_entries, file_history, indent=2)
            except ValueError as ve:
                raise ValueError(
                    _("Error decoding content of JSON history file {}").format(
                        history_path
                    )
                ) from ve
    except OSError as e:
        raise OSError(
            _("Unable to update entry in JSON history file {}").format(history_path)
        ) from e


def update_entry(history_path, command_info, index=None):
    """Update entry in history file. If index is None it updates a last entry.

    :param str history_path: path to the history log file
    :param dict command_info: command info entry for update
    :param int|None index: index of the command to be updated
    """
    if get_history_file_extension(history_path) != ".json":
        msg = "Updating entries is supported only for JSON format."
        raise ValueError(msg)
    _update_entry_in_JSON(history_path, command_info, index)


def copy(history_path, target_path):
    """Copy history file to the target location.

    :param str history_path: path to the history log file
    :param str target_path: target location for history log file
    :return boolean: True if file is successfully copied
    """
    try:
        shutil.copyfile(history_path, target_path)
    except OSError as e:
        raise OSError(
            _("Unable to copy history file {} to {}").format(history_path, target_path)
        ) from e
    return True
