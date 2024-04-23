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
from pathlib import Path

from datetime import datetime
import grass.script as gs
from grass.script.utils import parse_key_val


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
    extension = file_path.suffix
    return extension


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
    content_list = list()
    try:
        with open(
            history_path, encoding="utf-8", mode="r", errors="replace"
        ) as file_history:
            content_list = [
                {"command": line.strip(), "command_info": None}
                for line in file_history.readlines()
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
    content_list = list()
    try:
        with open(
            history_path, encoding="utf-8", mode="r", errors="replace"
        ) as file_history:
            content = file_history.read()
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


def _remove_entry_from_plain_text(history_path, index):
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
                if number not in [index]:
                    file_history.write(line)
    except OSError as e:
        raise OSError(
            _("Unable to remove entry from plain text history file {}").format(
                history_path
            )
        ) from e


def _remove_entry_from_JSON(history_path, index):
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


def remove_entry(history_path, index):
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

    # 2D raster MASK presence
    env = gs.gisenv(env_run)
    mapset_path = Path(env["GISDBASE"]) / env["LOCATION_NAME"] / env["MAPSET"]
    mask2d_present = (mapset_path / "cell" / "MASK").exists()

    # 3D raster MASK presence
    mask3d_present = (mapset_path / "grid3" / "RASTER3D_MASK").exists()

    # Computational region settings
    region_settings = dict(
        parse_key_val(
            gs.read_command("g.region", flags="g", env=env_run), val_type=float
        )
    )

    # Convert floats to integers if possible
    for key, value in region_settings.items():
        region_settings[key] = int(value) if value.is_integer() else value

    # Finalize the command info dictionary
    cmd_info = {
        "timestamp": exec_time,
        "mask2d": mask2d_present,
        "mask3d": mask3d_present,
        "region": region_settings,
    }
    return cmd_info


def _add_entry_to_JSON(history_path, entry):
    """Add entry to JSON history file.

    :param str history_path: path to the history log file
    :param dict entry: entry consisting of 'command' and 'command_info' keys
    """
    try:
        with open(history_path, encoding="utf-8", mode="r") as file_history:
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
    if get_history_file_extension(history_path) == ".json":
        _add_entry_to_JSON(history_path, entry)
    else:
        raise ValueError("Adding entries is supported only for JSON format.")


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
    if get_history_file_extension(history_path) == ".json":
        _update_entry_in_JSON(history_path, command_info, index)
    else:
        raise ValueError("Updating entries is supported only for JSON format.")


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
