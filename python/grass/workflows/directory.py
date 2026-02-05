#
# AUTHOR(S): Linda Karlovska <linda.karlovska@seznam.cz>
#
# PURPOSE:   Provides an interface for managing notebook working directory.
#
# COPYRIGHT: (C) 2025 by Linda Karlovska and the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.

"""
This module defines a class `JupyterDirectoryManager` that provides functionality
for working with Jupyter Notebook files stored within the current working directory.

Features:
- Creates a working directory if it does not exist
- Generates default template files
- Lists existing files in a working directory
- Imports files from external locations
- Exports files to external locations

Designed for use within GRASS GUI tools or scripting environments.
"""

import os
import json
import shutil
from pathlib import Path

import grass.script as gs


def get_default_jupyter_workdir():
    """
    Return the default working directory for Jupyter notebooks associated
    with the current GRASS mapset.
    :return: Path to the default notebook working directory (Path)
    """
    env = gs.gisenv()
    mapset_path = Path(env["GISDBASE"]) / env["LOCATION_NAME"] / env["MAPSET"]
    return mapset_path / "notebooks"


class JupyterDirectoryManager:
    """Manage a Jupyter notebook working directory."""

    def __init__(self, workdir=None, create_template=False):
        """Initialize the Jupyter notebook directory.

        :param workdir: Optional custom working directory (Path). If not provided,
                        the default working directory is used.
        :param create_template: If a welcome notebook should be created or not (bool).
        """
        self._workdir = workdir or get_default_jupyter_workdir()
        self._workdir.mkdir(parents=True, exist_ok=True)

        if not os.access(self._workdir, os.W_OK):
            raise PermissionError(
                _("Cannot write to the working directory: {}").format(self._workdir)
            )

        self._files = []
        self._create_template = create_template

    @property
    def workdir(self):
        """
        :return: path to the working directory (Path).
        """
        return self._workdir

    @property
    def files(self):
        """
        :return: List of file paths (list[Path])
        """
        return self._files

    def prepare_files(self):
        """
        Populate the list of files in the working directory.
        """
        # Find all .ipynb files in the notebooks directory
        self._files = [f for f in self._workdir.iterdir() if f.suffix == ".ipynb"]

        if self._create_template and not self._files:
            self.create_welcome_notebook()

    def import_file(self, source_path, new_name=None, overwrite=False):
        """Import an existing notebook file to the working directory.

        :param source_path: Path to the source .ipynb file to import (Path).
        :param new_name: New name for the imported file (with .ipynb extension),
                         if not provided, original filename is used ((Optional[str]))
        :param overwrite: Whether to overwrite an existing file with the same name (bool)
        :return: Path to the copied file in the working directory (Path)
        :raises FileNotFoundError: If the source_path does not exist
        :raises FileExistsError: If the target already exists and overwrite=False
        """
        # Validate the source path and ensure it has .ipynb extension
        source = Path(source_path)
        if not source.exists():
            raise FileNotFoundError(_("File not found: {}").format(source))
        if source.suffix != ".ipynb":
            raise ValueError(
                _("Source file must have .ipynb extension: {}").format(source)
            )

        # Ensure the working directory exists
        target_name = new_name or source.name
        if not target_name.endswith(".ipynb"):
            target_name += ".ipynb"

        # Create the target path in the working directory
        target_path = self._workdir / target_name

        # Check if the target file already exists
        if target_path.exists() and not overwrite:
            raise FileExistsError(
                _("Target file already exists: {}").format(target_path)
            )

        # Copy the source file to the target path
        shutil.copyfile(source, target_path)

        # Add the new target file to the list of files
        self._files.append(target_path)

        return target_path

    def export_file(self, file_name, destination_path, overwrite=False):
        """Export a file from the working directory to an external location.

        :param file_name: Name of the file (e.g., "example.ipynb") (str)
        :param destination_path: Full file path or target directory to export the file to (Path)
        :param overwrite: If True, allows overwriting an existing file at the destination (bool)
        :raises FileNotFoundError: If the source file does not exist or is not a .ipynb file
        :raises FileExistsError: If the destination file exists and overwrite is False
        """
        # Validate the file name and ensure it has .ipynb extension
        source_path = self._workdir / file_name
        if not source_path.exists() or source_path.suffix != ".ipynb":
            raise FileNotFoundError(_("File not found: {}").format(source_path))

        # Determine the destination path
        dest_path = Path(destination_path)
        if dest_path.is_dir() or dest_path.suffix != ".ipynb":
            dest_path /= file_name

        # Check if the destination file already exists
        if dest_path.exists() and not overwrite:
            raise FileExistsError(_("Target file already exists: {}").format(dest_path))

        # Create parent directories if they do not exist
        dest_path.parent.mkdir(parents=True, exist_ok=True)

        # Copy the file to the destination
        shutil.copyfile(source_path, dest_path)

    def create_welcome_notebook(self, file_name="welcome.ipynb"):
        """
        Create a welcome Jupyter notebook in the working directory with
        the placeholder '${NOTEBOOK_DIR}' replaced by the actual path.

        :param file_name: Name of the template file to copy (str)
        :return: Path to the created template file (Path)
        """
        # Copy template file to the working directory
        template_path = Path(__file__).parent / "template_notebooks" / file_name
        template_copy = self.import_file(template_path)

        # Load the template file
        content = template_copy.read_text(encoding="utf-8")

        # Replace the placeholder '${NOTEBOOK_DIR}' with actual working directory path
        content = content.replace(
            "${NOTEBOOK_DIR}", str(self._workdir).replace("\\", "/")
        )

        # Save the modified content back to the template file
        template_copy.write_text(content, encoding="utf-8")
        return template_copy

    def create_new_notebook(self, new_name, template_name="new.ipynb"):
        """
        Create a new Jupyter notebook in the working directory using a specified template.

        This method copies the content of a template notebook (default: 'new.ipynb')
        and saves it as a new file with the user-defined name in the current working directory.

        :param new_name: Desired filename of the new notebook (must end with '.ipynb',
                        or it will be automatically appended) (str).
        :param template_name: Name of the template file to use (default: 'new.ipynb') (str).
        :return: Path to the newly created notebook (Path).
        :raises ValueError: If the provided name is empty.
        :raises FileExistsError: If a notebook with the same name already exists.
        :raises FileNotFoundError: If the specified template file does not exist.
        """
        if not new_name:
            raise ValueError(_("Notebook name must not be empty"))

        if not new_name.endswith(".ipynb"):
            new_name += ".ipynb"

        target_path = self.workdir / new_name

        if target_path.exists():
            raise FileExistsError(_("File '{}' already exists").format(new_name))

        # Load the template notebook content
        template_path = Path(__file__).parent / "template_notebooks" / template_name
        with open(template_path, encoding="utf-8") as f:
            content = json.load(f)

        # Save the content to the new notebook file
        with open(target_path, "w", encoding="utf-8") as f:
            json.dump(content, f, indent=2)

        # Register the new file internally
        self._files.append(target_path)

        return target_path
