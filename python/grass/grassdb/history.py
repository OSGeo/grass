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
from abc import ABC, abstractmethod

from grass.script import gisenv


def get_current_mapset_gui_history_path():
    """Return path to the current mapset history file.
    The standard long-term format is plain text.
    Newly it can be also stored as JSON-formatted.
    """
    env = gisenv()
    base_path = os.path.join(env["GISDBASE"], env["LOCATION_NAME"], env["MAPSET"])
    history_filename = ".wxgui_history"

    txt_path = os.path.join(base_path, history_filename)
    json_path = os.path.join(base_path, f"{history_filename}.json")

    # Return path to plain-text file if exists otherwise return JSON
    return txt_path if os.path.exists(txt_path) else json_path


def create_history_manager():
    """Factory method which creates the class
    for managing existing history files based on the file format type.
    """
    history_path = get_current_mapset_gui_history_path()
    file_suffix = Path(history_path).suffix.lower()

    if file_suffix == ".json":
        return HistoryJSONManager(history_path, "json")
    else:
        return HistoryPlainTextManager(history_path, "plain")


class HistoryManager(ABC):
    """Abstract class for managing history log file."""

    def __init__(self, history_path, filetype):
        """Initialize HistoryManager with the given history file path and filetype."""
        self.history_path = history_path
        self.filetype = filetype

    @abstractmethod
    def get_content(self):
        pass

    @abstractmethod
    def add_entry_to_history(self, command, command_info=None):
        pass

    @abstractmethod
    def remove_entry_from_history(self, entry):
        pass

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


class HistoryPlainTextManager(HistoryManager):
    """Manager for plain text history log.
    It ensures the backward compability
    of existing mapsets that include history log as plain text.
    """

    def get_content(self):
        """Get content of history file as a list of dicts
        with 'command' and 'command_info' keys.
        'command_info' is always None since plain text history file
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
                _("Unable to read content from history file {}").format(
                    self.history_path
                )
            ) from e
        return content_list

    def add_entry_to_history(self, entry):
        """Add entry to history file. It always appends to the existing plain text file.

        :param dict entry: entry consisting of 'command' and 'command_info' keys
        """
        try:
            with open(self.history_path, encoding="utf-8", mode="a") as file_history:
                file_history.write(entry["command"] + "\n")
        except OSError as e:
            raise OSError(
                _("Unable to add entry to history file {}").format(self.history_path)
            ) from e

    def remove_entry_from_history(self, index):
        """Remove entry from history file.

        :param int index: line number of the command to be removed
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
                _("Unable to remove entry from history file {}").format(
                    self.history_path
                )
            ) from e


class HistoryJSONManager(HistoryManager):
    """Manager for currently used command history log format."""

    def __init__(self, history_path, filetype):
        super().__init__(history_path, filetype)
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

    def get_content(self):
        """Get content of history file as a list of dictionaries
        with 'command' and 'command_info' keys."""
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
                            _("Error decoding JSON content in history file {}").format(
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
                _("Unable to read content from history file {}").format(
                    self.history_path
                )
            ) from e
        return content_list

    def add_entry_to_history(self, entry):
        """Add entry to the list of dictionaries in the history file.

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
                _("Error decoding JSON content in history file {}").format(
                    self.history_path
                )
            ) from ve
        except OSError as e:
            raise OSError(
                _("Unable to add entry to history file {}").format(self.history_path)
            ) from e

    def remove_entry_from_history(self, index):
        """Remove entry from history file.

        :param int index: index of the command which should be removed
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
                        _("Error decoding JSON content in history file {}").format(
                            self.history_path
                        )
                    ) from ve
        except OSError as e:
            raise OSError(
                _("Unable to add entry to history file {}").format(self.history_path)
            ) from e

    def update_entry_in_history(self, command_info, index=None):
        """Update entry in history file. If index is None it updates a last entry.

        :param dict command_info: command info entry for update
        :param int|None index: index of the command which should be updated
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
                        _("Error decoding JSON content in history file {}").format(
                            self.history_path
                        )
                    ) from ve
        except OSError as e:
            raise OSError(
                _("Unable to add entry to history file {}").format(self.history_path)
            ) from e

    def get_initial_command_info(self, env_run):
        """Get information about the launched command.

        :param env_run: environment needed for processing grass command
        :return cmd_info dict: initial information about the launched command
        """
        from datetime import datetime
        import grass.script as grass

        # Execution timestamp in ISO 8601 format
        exec_time = datetime.now().isoformat()

        # 2D mask set
        env = gisenv()
        mapset_path = os.path.join(env["GISDBASE"], env["LOCATION_NAME"], env["MAPSET"])
        mask2d_set = os.path.exists(os.path.join(mapset_path, "cell", "MASK"))

        # Computational region settings
        region_settings_g = grass.read_command("g.region", flags="g", env=env_run)

        # Convert region settings output to dictionary with float/int values
        region_settings = {}
        for line in region_settings_g.split("\n"):
            parts = line.split("=")
            if len(parts) == 2:
                key = parts[0]
                value_str = parts[1]
                try:
                    value = float(value_str)
                    if value.is_integer():
                        value = int(value)
                except ValueError:
                    pass
                region_settings[key] = value

        # Finalize the command info dictionary
        cmd_info = {
            "timestamp": exec_time,
            "mask": mask2d_set,
            "region": region_settings,
        }
        return cmd_info
