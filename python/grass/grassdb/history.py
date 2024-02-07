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
from pathlib import Path

from grass.script import gisenv


def get_current_mapset_gui_history_path():
    """Return path to the current mapset history file.
    The standard long-term format is plain text.
    Newly it is stored as JSON-formatted.
    """
    env = gisenv()
    base_path = os.path.join(env["GISDBASE"], env["LOCATION_NAME"], env["MAPSET"])
    history_filename = ".wxgui_history"

    txt_path = os.path.join(base_path, history_filename)
    json_path = os.path.join(base_path, f"{history_filename}.json")

    # Return path to plain-text file if exists otherwise return JSON
    return txt_path if os.path.exists(txt_path) else json_path


def create_history_manager():
    """Prepares the instance of the class
    for managing existing history files based on the file format type.
    """
    history_path = get_current_mapset_gui_history_path()
    file_suffix = Path(history_path).suffix.lower()
    history_manager = HistoryManager(history_path)
    history_manager.set_filetype(
        "json"
    ) if file_suffix == ".json" else history_manager.set_filetype("plain")
    return history_manager


class HistoryManager:
    """Class for managing history log file.
    This API is capable of working with command history
    in both plain text and JSON formats."""

    def __init__(self, history_path):
        self.history_path = history_path
        self.filetype = None

        if not os.path.exists(history_path):
            self.create_history_file()

    def create_history_file(self):
        """Set up a new GUI history file which is always JSON-formatted."""
        try:
            with open(self.history_path, encoding="utf-8", mode="w"):
                pass
        except OSError as e:
            raise OSError(
                _("Unable to create history file {}").format(self.history_path)
            ) from e

    def set_filetype(self, filetype):
        """Set the filetype to 'plain' or 'json'."""
        if filetype.lower() in ["plain", "json"]:
            self.filetype = filetype
        else:
            raise ValueError("Invalid filetype. Supported options: 'plain' or 'json'.")

    def change_history_path_to_JSON(self):
        """Change history path to JSON path."""
        # Extract file path and name without extension
        file_path, file_name = os.path.split(self.history_path)
        file_name_no_ext, _ = os.path.splitext(file_name)

        # JSON file path
        self.history_path = os.path.join(file_path, f"{file_name_no_ext}.json")
        self.filetype = "json"

    def _read_from_PT(self):
        """Read content of plain text history file.
        :return content_list: list of dictionaries
        with 'command' and 'command_info' keys
        'command_info' is always empty since plain text history file
        stores only executed commands."""
        content_list = list()
        try:
            with open(
                self.history_path, encoding="utf-8", mode="r", errors="replace"
            ) as file_history:
                content_list = [
                    {"command": line.strip(), "command_info": None}
                    for line in file_history.readlines()
                ]
        except OSError as e:
            raise OSError(
                _("Unable to read from plain text history file {}").format(
                    self.history_path
                )
            ) from e
        return content_list

    def _read_from_JSON(self):
        """Read content of JSON history file.
        :return content_list: list of dictionaries
        with 'command' and 'command_info' keys
        """
        content_list = list()
        try:
            with open(
                self.history_path, encoding="utf-8", mode="r", errors="replace"
            ) as file_history:
                content = file_history.read()
                if content:
                    try:
                        history_entries = json.loads(content)
                    except ValueError as ve:
                        raise ValueError(
                            _("Error decoding content of JSON history file {}").format(
                                self.history_path
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
                _("Unable to read from JSON history file {}").format(self.history_path)
            ) from e
        return content_list

    def read(self):
        """Read the content of the history file.
        :return content_list: list of dictionaries
        with 'command' and 'command_info' keys
        """
        if self.filetype == "plain":
            return self._read_from_PT()
        elif self.filetype == "json":
            return self._read_from_JSON()

    def _remove_entry_from_PT(self, index):
        """Remove entry from plain-text history file.

        :param int index: index of the command to be removed
        """
        try:
            with open(self.history_path, encoding="utf-8", mode="r+") as file_history:
                lines = file_history.readlines()
                file_history.seek(0)
                file_history.truncate()
                for number, line in enumerate(lines):
                    if number not in [index]:
                        file_history.write(line)
        except OSError as e:
            raise OSError(
                _("Unable to remove entry from plain text history file {}").format(
                    self.history_path
                )
            ) from e

    def _remove_entry_from_JSON(self, index):
        """Remove entry from JSON history file.

        :param int index: index of the command to be removed
        """
        try:
            with open(self.history_path, encoding="utf-8", mode="r+") as file_history:
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
                            self.history_path
                        )
                    ) from ve
        except OSError as e:
            raise OSError(
                _("Unable to remove entry from JSON history file {}").format(
                    self.history_path
                )
            ) from e

    def remove_entry(self, index):
        """Remove entry from history file.

        :param int index: index of the command to be removed
        """
        if self.filetype == "plain":
            self._remove_entry_from_PT(index)
        elif self.filetype == "json":
            self._remove_entry_from_JSON(index)

    def convert_PT_to_JSON(self):
        """Convert plain text history log to JSON format."""
        try:
            lines = self._read_from_PT()

            # Extract file path and name without extension
            file_path, file_name = os.path.split(self.history_path)
            file_name_no_ext, _ = os.path.splitext(file_name)

            # JSON file path
            json_file = os.path.join(file_path, f"{file_name_no_ext}.json")

            # Write JSON data to a new file
            with open(json_file, "w") as json_outfile:
                json.dump(lines, json_outfile, indent=2)

            # Remove the old plain text file
            os.remove(self.history_path)

            # Set history path to JSON file
            self.history_path = json_file
            self.filetype = "json"

        except OSError as e:
            raise OSError(
                _("Unable to convert plain text history file {} to JSON format").format(
                    self.history_path
                )
            ) from e

    def get_initial_command_info(self, env_run):
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
        mask3d_present = os.path.exists(
            os.path.join(mapset_path, "grid3", "RASTER3D_MASK")
        )

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

    def _add_entry_to_JSON(self, entry):
        """Add entry to JSON history file.

        :param dict entry: entry consisting of 'command' and 'command_info' keys
        """
        try:
            with open(self.history_path, encoding="utf-8", mode="r") as fileHistory:
                existing_data = json.load(fileHistory)
        except (OSError, ValueError):
            existing_data = []

        existing_data.append(entry)
        try:
            with open(self.history_path, encoding="utf-8", mode="w") as fileHistory:
                json.dump(existing_data, fileHistory, indent=2)
        except ValueError as ve:
            raise ValueError(
                _("Error decoding content of JSON history file {}").format(
                    self.history_path
                )
            ) from ve
        except OSError as e:
            raise OSError(
                _("Unable to add entry to JSON history file {}").format(
                    self.history_path
                )
            ) from e

    def add_entry(self, entry):
        """Add entry to history file.

        :param dict entry: entry consisting of 'command' and 'command_info' keys
        """
        if self.filetype == "json":
            self._add_entry_to_JSON(entry)
        else:
            raise ValueError("Adding entries is supported only for JSON format.")

    def _update_entry_in_JSON(self, command_info, index=None):
        """Update entry in JSON history file. If index is None it updates a last entry.

        :param dict command_info: command info entry for update
        :param int|None index: index of the command to be updated
        """
        try:
            with open(self.history_path, encoding="utf-8", mode="r+") as file_history:
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
                            self.history_path
                        )
                    ) from ve
        except OSError as e:
            raise OSError(
                _("Unable to update entry in JSON history file {}").format(
                    self.history_path
                )
            ) from e

    def update_entry(self, command_info, index=None):
        """Update entry in history file. If index is None it updates a last entry.

        :param dict command_info: command info entry for update
        :param int|None index: index of the command to be updated
        """
        if self.filetype == "json":
            self._update_entry_in_JSON(command_info, index)
        else:
            raise ValueError("Updating entries is supported only for JSON format.")

    def copy_history(self, target_path):
        """Copy history file to the target location.
        Returns True if file is successfully copied."""
        try:
            shutil.copyfile(self.history_path, target_path)
        except OSError as e:
            raise OSError(
                _("Unable to copy file {} to {}").format(self.history_path, target_path)
            ) from e
        return True
