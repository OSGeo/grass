##############################################################################
# AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>
#
# PURPOSE:   API to call GRASS tools as Python functions without a session
#
# COPYRIGHT: (C) 2025 Vaclav Petras and the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.
##############################################################################

"""
An API to call GRASS tools as Python functions without a session

This is not a stable part of the API. Use at your own risk.
"""

import tempfile
import json
import subprocess
import tarfile
import weakref
from pathlib import Path

import grass.script as gs
from .tools import Tools, PackImporterExporter


# Using inheritance to get the getattr behavior and other functionality,
# but the session and env really make it more seem like a case for composition.
class StandaloneTools(Tools):
    def __init__(self, session=None, work_dir=None, errors=None, capture_output=True):
        super().__init__(
            overwrite=False,
            quiet=False,
            verbose=False,
            superquiet=False,
            stdin=None,
            errors=errors,
            capture_output=capture_output,
        )
        self._crs_initialized = False
        self._work_dir = work_dir
        self._tmp_dir = None
        self._tmp_dir_finalizer = None
        self._session = session
        if session:
            # If session is provided, we will use it as is.
            self._crs_initialized = True
        # Because we don't setup a session here, we don't have runtime available for
        # tools to be called through method calls. Should we just start session here
        # to have the runtime?
        self._errors = errors
        self._capture_output = capture_output

    def run(self, name, /, *, flags=None, **kwargs):
        return self.run_from_list(gs.make_command(name, flags=flags, **kwargs))

    # Make this an overload of run.
    # Or at least use the same signature as the parent class.
    def run_from_list(self, command):
        """

        Passing --help to this function will not work.
        """
        if not self._session:
            # We create session and an empty XY project in one step.
            self._create_session()

        parameters = json.loads(
            subprocess.check_output(
                [*command, "--json"], text=True, env=self._session.env
            )
        )

        pack_importer_exporter = PackImporterExporter(env=self._session.env)
        pack_importer_exporter.modify_and_ingest_argument_list(command, parameters)

        if not self._crs_initialized:
            self._initialize_crs(pack_importer_exporter.input_rasters)

        pack_importer_exporter.import_data()

        if pack_importer_exporter.input_rasters:
            # Reset the region for every run or keep it persistent?
            # Also, we now set that even for an existing session, this is
            # consistent with behavior without a provided session.
            # We could use env to pass the regions which would allow for the change
            # while not touching the underlying session.
            gs.run_command(
                "g.region",
                raster=pack_importer_exporter.input_rasters[0].stem,
                env=self._session.env,
            )

        result = super().run_from_list(command, env=self._session.env)

        pack_importer_exporter.export_data()

        return result

    def _create_session(self):
        # Temporary folder for all our files
        if self._work_dir:
            base_dir = self._work_dir
        else:
            # Resource is managed by weakref.finalize.
            self._tmp_dir = (
                # pylint: disable=consider-using-with
                tempfile.TemporaryDirectory()
            )

            def cleanup(tmpdir):
                tmpdir.cleanup()

            self._tmp_dir_finalizer = weakref.finalize(self, cleanup, self._tmp_dir)
            base_dir = self._tmp_dir.name
        project_name = "project"
        project_path = Path(base_dir) / project_name
        gs.create_project(project_path)
        self._session = gs.setup.init(project_path)

    def _initialize_crs(self, rasters):
        # Get the mapset path
        mapset_path = gs.read_command(
            "g.gisenv", get="GISDBASE,LOCATION_NAME,MAPSET", sep="/"
        )
        mapset_path = Path(mapset_path.strip())

        if rasters:
            with tarfile.TarFile(rasters[0]) as tar:
                for name in [
                    "PROJ_UNITS",
                    "PROJ_INFO",
                    "PROJ_EPSG",
                    "PROJ_SRID",
                    "PROJ_WKT",
                ]:
                    try:
                        tar_info = tar.getmember(name)
                    except KeyError:
                        continue
                    Path(mapset_path / name).write_bytes(
                        tar.extractfile(tar_info).read()
                    )

    def cleanup(self):
        if self._tmp_dir_finalizer:
            self._tmp_dir_finalizer()

    def __enter__(self):
        """Enter the context manager context.

        Notably, the session is activated using the *init* function.

        :returns: reference to the object (self)
        """
        return self

    def __exit__(self, type, value, traceback):
        """Exit the context manager context.

        Finishes the existing session.
        """
        self.cleanup()
