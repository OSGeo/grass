"""
Managing existing history files included within mapset

(C) 2023 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

.. sectionauthor:: Linda Karlovska (Kladivova) linda.karlovska@seznam.cz
"""

import os
import json
import shutil

from grass.script import gisenv


def get_current_mapset_gui_history_path():
    """Return path to the current mapset history file."""
    env = gisenv()
    base_path = os.path.join(env["GISDBASE"], env["LOCATION_NAME"], env["MAPSET"])
    history_filename = ".wxgui_history"

    txt_path = os.path.join(base_path, history_filename)
    json_path = os.path.join(base_path, f"{history_filename}.json")

    # Return path to plain-text file if exists otherwise return JSON
    return txt_path if os.path.exists(txt_path) else json_path


def get_extension():
    """Return extension of the current mapset history file.
    The standard long-term format is plain text.
    Newly it is stored as JSON-formatted.
    """
    history_path = get_current_mapset_gui_history_path()
    file_path, file_name = os.path.split(history_path)
    file_name_no_ext, extension = os.path.splitext(file_name)
    return extension


def create_history_file():
    """Set up a new GUI history file which is always JSON-formatted."""
    history_path = get_current_mapset_gui_history_path()
    if not os.path.exists(history_path):
        try:
            with open(history_path, encoding="utf-8", mode="w"):
                pass
        except OSError as e:
            raise OSError(
                _("Unable to create history file {}").format(history_path)
            ) from e


def _read_from_PT(history_path):
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


def read():
    """Read the content of the history file.

    :return content_list: list of dictionaries
    with 'command' and 'command_info' keys
    """
    history_path = get_current_mapset_gui_history_path()
    if get_extension() == ".json":
        return _read_from_JSON(history_path)
    else:
        return _read_from_PT(history_path)


def _remove_entry_from_PT(history_path, index):
    """Remove entry from plain-text history file.

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


def remove_entry(index):
    """Remove entry from history file.

    :param int index: index of the command to be removed
    """
    history_path = get_current_mapset_gui_history_path()
    if get_extension() == ".json":
        _remove_entry_from_JSON(history_path, index)
    else:
        _remove_entry_from_PT(history_path, index)


def convert_PT_to_JSON():
    """Convert plain text history log to JSON format."""
    if get_extension() != ".json":
        try:
            history_path = get_current_mapset_gui_history_path()
            lines = _read_from_PT(history_path)

            # Extract file path and name without extension
            file_path, file_name = os.path.split(history_path)
            file_name_no_ext, _ = os.path.splitext(file_name)

            # JSON file path
            json_file = os.path.join(file_path, f"{file_name_no_ext}.json")

            # Write JSON data to a new file
            with open(json_file, "w") as json_outfile:
                json.dump(lines, json_outfile, indent=2)

            # Remove the old plain text file
            os.remove(history_path)

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
    from datetime import datetime
    import grass.script as grass
    from grass.script.utils import parse_key_val

    # Execution timestamp in ISO 8601 format
    exec_time = datetime.now().isoformat()

    # 2D raster MASK presence
    env = gisenv()
    mapset_path = os.path.join(env["GISDBASE"], env["LOCATION_NAME"], env["MAPSET"])
    mask2d_present = os.path.exists(os.path.join(mapset_path, "cell", "MASK"))

    # 3D raster MASK presence
    mask3d_present = os.path.exists(os.path.join(mapset_path, "grid3", "RASTER3D_MASK"))

    # Computational region settings
    region_settings = dict(
        parse_key_val(
            grass.read_command("g.region", flags="g", env=env_run), val_type=float
        )
    )

    # Convert floats to integers if possible
    for key, value in region_settings.items():
        if value.is_integer():
            region_settings[key] = int(value)

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
        with open(history_path, encoding="utf-8", mode="r") as fileHistory:
            existing_data = json.load(fileHistory)
    except (OSError, ValueError):
        existing_data = []

    existing_data.append(entry)
    try:
        with open(history_path, encoding="utf-8", mode="w") as fileHistory:
            json.dump(existing_data, fileHistory, indent=2)
    except ValueError as ve:
        raise ValueError(
            _("Error decoding content of JSON history file {}").format(history_path)
        ) from ve
    except OSError as e:
        raise OSError(
            _("Unable to add entry to JSON history file {}").format(history_path)
        ) from e


def add_entry(entry):
    """Add entry to history file.

    :param dict entry: entry consisting of 'command' and 'command_info' keys
    """
    if get_extension() == ".json":
        history_path = get_current_mapset_gui_history_path()
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


def update_entry(command_info, index=None):
    """Update entry in history file. If index is None it updates a last entry.

    :param dict command_info: command info entry for update
    :param int|None index: index of the command to be updated
    """
    if get_extension() == ".json":
        history_path = get_current_mapset_gui_history_path()
        _update_entry_in_JSON(history_path, command_info, index)
    else:
        raise ValueError("Updating entries is supported only for JSON format.")


def copy(target_path):
    """Copy history file to the target location.
    Returns True if file is successfully copied."""
    try:
        history_path = get_current_mapset_gui_history_path()
        shutil.copyfile(history_path, target_path)
    except OSError as e:
        raise OSError(
            _("Unable to copy history file {} to {}").format(history_path, target_path)
        ) from e
    return True
