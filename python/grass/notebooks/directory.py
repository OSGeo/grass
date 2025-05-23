#
# AUTHOR(S): Linda Karlovska <linda.karlovska@seznam.cz>
#
# PURPOSE:   Provides a class for managing notebook files within the current
#            GRASS mapset.
#
# COPYRIGHT: (C) 2025 by Linda Karlovska and the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.

"""
This module defines a class `NotebookDirectoryManager` that provides functionality
for working with Jupyter Notebook files stored within the current GRASS mapset.
It handles:

- Creating a notebooks directory if it does not exist
- Generating a default template notebook
- Listing existing `.ipynb` files
- Optionally importing notebooks from external locations

Designed for use within GRASS GUI tools or scripting environments.
"""

import shutil
from pathlib import Path

import grass.script as gs


class NotebookDirectoryManager:
    """Manage a directory of Jupyter notebooks tied to the current GRASS mapset.

    Handles locating the notebook directory, listing existing notebooks,
    and creating a default template notebook if none exist.
    """

    def __init__(self):
        """Initialize the notebook directory and load existing notebooks."""
        self._notebook_workdir = self._get_notebook_workdir()
        self._notebook_files = None

    @property
    def notebook_workdir(self):
        """Path to the notebook working directory."""
        return self._notebook_workdir

    @property
    def notebook_files(self):
        """list of all .ipynb files in the current mapset notebooks dir."""
        return self._notebook_files

    def _get_notebook_workdir(self):
        """Return path to the current mapset notebook directory.
        It is created if it does not exist.
        """
        env = gs.gisenv()
        mapset_path = "{gisdbase}/{location}/{mapset}".format(
            gisdbase=env["GISDBASE"],
            location=env["LOCATION_NAME"],
            mapset=env["MAPSET"],
        )
        notebook_workdir = Path(mapset_path) / "notebooks"
        notebook_workdir.mkdir(parents=True, exist_ok=True)
        return notebook_workdir

    def prepare_notebook_files(self):
        """Return list of all .ipynb files in the current mapset notebooks dir.
        The template file is created if no ipynb files are found.
        """
        # Find all .ipynb files in the notebooks directory
        self._notebook_files = [
            f for f in self._notebook_workdir.iterdir() if f.suffix == ".ipynb"
        ]
        print(self._notebook_files)

        if not self._notebook_files:
            # If no .ipynb files are found, create a template ipynb file
            self._notebook_files.append(self.create_template())
        print(self._notebook_files)

    def copy_notebook(self, source_path, new_name=None, overwrite=False):
        """Copy an existing Jupyter notebook file into the notebook directory.

        :param source_path: Path to the source .ipynb notebook
        :param new_name: Optional new name for the copied notebook (with .ipynb extension),
                        if not provided, original filename is used
        :param overwrite: Whether to overwrite an existing file with the same name
        :return: Path to the copied notebook
        :raises FileNotFoundError: If the source_path does not exist
        :raises FileExistsError: If the target already exists and overwrite=False
        """
        source = Path(source_path)
        if not source.exists() or not source.suffix == ".ipynb":
            raise FileExistsError(_("Notebook file not found:: {}").format(source))

        target_name = new_name or source.name
        target_path = self._notebook_workdir / target_name

        if target_path.exists() and not overwrite:
            raise FileExistsError(
                _("Target notebook already exists: {}").format(target_path)
            )

        shutil.copyfile(source, target_path)
        return target_path

    def create_template(self, filename="template.ipynb"):
        """
        Create a template Jupyter notebook by copying an existing template
        file and replacing workdir placeholder.
        :param filename: Name of the template file to copy
        :return: Path to the created template notebook
        """
        # Copy template file to the notebook directory
        notebook_template_path = self.copy_notebook(
            Path(__file__).parent / "template_notebooks" / filename
        )
        print(notebook_template_path)

        # Load the template file
        with open(notebook_template_path, encoding="utf-8"):
            content = Path(notebook_template_path).read_text(encoding="utf-8")

        # Replace the placeholder with the actual notebook workdir
        content = content.replace("XXX", str(self._notebook_workdir).replace("\\", "/"))

        # Save the modified content back to the template file
        with open(notebook_template_path, "w", encoding="utf-8"):
            Path(notebook_template_path).write_text(content, encoding="utf-8")

        # Add the new template file to the list of notebook files
        self._notebook_files.append(notebook_template_path)

        return notebook_template_path
