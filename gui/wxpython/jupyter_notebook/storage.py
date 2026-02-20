"""
@package jupyter_notebook.storage

@brief Simple interface for working with Jupyter Notebook files stored within
the current storage.

Classes:
 - storage::JupyterStorageManager

(C) 2026 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Linda Karlovska <linda.karlovska seznam.cz>
"""

import os
import shutil
from pathlib import Path

import grass.script as gs
from .utils import get_project_jupyter_storage


# Template notebook filenames
WELCOME_NOTEBOOK_NAME = "welcome.ipynb"
NEW_NOTEBOOK_TEMPLATE_NAME = "new.ipynb"


class JupyterStorageManager:
    """Manage Jupyter notebook storage."""

    def __init__(
        self, storage: Path | None = None, create_template: bool = False
    ) -> None:
        """Initialize the Jupyter notebook storage.

        :param storage: Optional custom storage path. If not provided,
                        the project storage is used
        :param create_template: If a welcome notebook should be created or not
        :raises PermissionError: If the storage is not writable
        """
        self._storage = storage or get_project_jupyter_storage()
        self._storage.mkdir(parents=True, exist_ok=True)

        if not os.access(self._storage, os.W_OK):
            msg = "Cannot write to the storage: {}"
            raise PermissionError(_(msg).format(self._storage))

        self._files = []
        self._create_template = create_template

    @property
    def storage(self) -> Path:
        """Return path to the storage."""
        return self._storage

    @property
    def files(self) -> list[Path]:
        """Return list of notebook files."""
        return self._files

    def prepare_files(self) -> None:
        """Populate the list of files in the storage."""
        # Find all .ipynb files in the storage
        self._files = [f for f in self._storage.iterdir() if f.suffix == ".ipynb"]

        if self._create_template and not self._files:
            self.create_welcome_notebook()

    def import_file(
        self, source_path: Path, new_name: str | None = None, overwrite: bool = False
    ) -> Path:
        """Import an existing notebook file to the storage.

        :param source_path: Path to the source .ipynb file to import
        :param new_name: New name for the imported file (with .ipynb extension),
                         if not provided, original filename is used
        :param overwrite: Whether to overwrite an existing file with the same name
        :return: Path to the copied file in the storage
        :raises FileNotFoundError: If the source_path does not exist
        :raises FileExistsError: If the target already exists and overwrite=False
        :raises ValueError: If source file doesn't have .ipynb extension
        """
        # Validate the source path and ensure it has .ipynb extension
        source = Path(source_path)
        if not source.exists():
            raise FileNotFoundError(_("File not found: {}").format(source))
        if source.suffix != ".ipynb":
            raise ValueError(
                _("Source file must have .ipynb extension: {}").format(source)
            )

        # Determine target name
        target_name = new_name or source.name
        if not target_name.endswith(".ipynb"):
            target_name += ".ipynb"

        # Create the target path in the storage
        target_path = self._storage / target_name

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

    def export_file(
        self, file_name: str, destination_path: Path, overwrite: bool = False
    ) -> None:
        """Export a file from the storage to an external location.

        :param file_name: Name of the file (e.g., "example.ipynb")
        :param destination_path: Full file path or target location to export the file to
        :param overwrite: If True, allows overwriting an existing file at the destination
        :raises FileNotFoundError: If the source file does not exist or is not a .ipynb file
        :raises FileExistsError: If the destination file exists and overwrite is False
        """
        # Validate the file name and ensure it has .ipynb extension
        source_path = self._storage / file_name
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

    def _create_from_template(
        self,
        template_name: str,
        target_name: str | None = None,
        replacements: dict[str, str] | None = None,
    ) -> Path:
        """Create a notebook from a template and optionally replace placeholders.

        :param template_name: Template filename located in ``template_notebooks``
        :param target_name: Optional target filename for the new notebook
        :param replacements: Optional mapping of placeholder strings to replacement values
        :return: Path to the created notebook file
        :raises FileExistsError: If target file already exists
        """
        # Locate the template file inside the package
        template_path = Path(__file__).parent / "template_notebooks" / template_name

        # Determine target path (copy vs. create new file)
        if target_name is None:
            target_path = self.import_file(template_path)
        else:
            target_path = self.storage / target_name

            # Prevent accidental overwrite
            if target_path.exists():
                raise FileExistsError(_("File '{}' already exists").format(target_name))

        # Load template content as plain text
        content = template_path.read_text(encoding="utf-8")

        # Replace placeholders if provided
        if replacements:
            for key, value in replacements.items():
                content = content.replace(key, value)

        # Write the processed content to the target file
        target_path.write_text(content, encoding="utf-8")

        return target_path

    def create_welcome_notebook(
        self, template_name: str = WELCOME_NOTEBOOK_NAME
    ) -> Path:
        """Create a welcome notebook with storage and mapset path placeholders replaced.

        :param template_name: Template filename
        :return: Path to the created notebook
        """
        # Prepare placeholder replacements
        env = gs.gisenv()
        mapset_path = Path(env["GISDBASE"], env["LOCATION_NAME"], env["MAPSET"])
        replacements = {
            "${STORAGE_PATH}": str(self._storage).replace("\\", "/"),
            "${MAPSET_PATH}": str(mapset_path).replace("\\", "/"),
        }

        # Create notebook from template
        return self._create_from_template(template_name, replacements=replacements)

    def create_new_notebook(
        self, new_name: str, template_name: str = NEW_NOTEBOOK_TEMPLATE_NAME
    ) -> Path:
        """Create a new notebook from a template with only mapset path placeholder replaced.

        :param new_name: Desired notebook filename
        :param template_name: Template filename
        :return: Path to the created notebook
        :raises ValueError: If name is empty
        :raises FileExistsError: If file already exists
        """
        # Validate notebook name
        if not new_name:
            msg = "Notebook name must not be empty"
            raise ValueError(_(msg))

        # Ensure .ipynb extension
        if not new_name.endswith(".ipynb"):
            new_name += ".ipynb"

        # Replace only mapset placeholder
        env = gs.gisenv()
        mapset_path = Path(env["GISDBASE"], env["LOCATION_NAME"], env["MAPSET"])
        replacements = {
            "${MAPSET_PATH}": str(mapset_path).replace("\\", "/"),
        }

        # Create notebook from template under the new name
        return self._create_from_template(template_name, new_name, replacements)
