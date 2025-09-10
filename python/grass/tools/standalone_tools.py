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
from .session_tools import Tools
from .support import ParameterConverter, ToolFunctionResolver
from grass.tools.importexport import ImporterExporter


# Using inheritance to get the getattr behavior and other functionality,
# but the session and env really make it more seem like a case for composition.
class StandaloneTools:
    def __init__(self, session=None, work_dir=None, errors=None, capture_output=True):
        self._tools = None
        self._errors = errors
        self._capture_output = capture_output
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
        self._region_is_set = False
        self._region_file = None
        self._region_modified_time = None
        self._errors = errors
        self._capture_output = capture_output
        self._name_helper = None

    def _process_parameters(self, command, popen_options):
        if not self._session:
            # We create session and an empty XY project in one step.
            self._create_session()

        env = popen_options.get("env", self._session.env)

        return subprocess.run(
            [*command, "--json"], text=True, capture_output=True, env=env
        )

    def run(self, name, /, **kwargs):
        object_parameter_handler = ParameterConverter()
        object_parameter_handler.process_parameters(kwargs)

        args, popen_options = gs.popen_args_command(name, **kwargs)

        interface_result = self._process_parameters(args, popen_options)
        if interface_result.returncode != 0:
            # This is only for the error states.
            return gs.handle_errors(
                interface_result.returncode,
                result=None,
                args=[name],
                kwargs=kwargs,
                stderr=interface_result.stderr,
                handler="raise",
            )
        parameters = json.loads(interface_result.stdout)

        if not self._session:
            # We create session and an empty XY project in one step.
            self._create_session()

        rows_columns = object_parameter_handler.input_rows_columns()
        if not self._is_region_modified() and rows_columns:
            # Reset the region for every run or keep it persistent?
            # Also, we now set that even for an existing session, this is
            # consistent with behavior without a provided session.
            # We could use env to pass the regions which would allow for the change
            # while not touching the underlying session.
            rows, cols = rows_columns
            self.no_nonsense_run(
                "g.region",
                rows=rows,
                cols=cols,
                env=self._session.env,
            )
            self._region_is_set = True

        object_parameter_handler.translate_objects_to_data(
            kwargs, parameters, env=self._session.env
        )

        # We approximate tool_kwargs as original kwargs.
        result = self.run_from_list(
            args,
            tool_kwargs=kwargs,
            processed_parameters=parameters,
            stdin=object_parameter_handler.stdin,
            **popen_options,
        )
        use_objects = object_parameter_handler.translate_data_to_objects(
            kwargs, parameters, env=self._session.env
        )
        if use_objects:
            result = object_parameter_handler.result
        return result

    # Make this an overload of run.
    # Or at least use the same signature as the parent class.
    def run_from_list(
        self,
        command,
        tool_kwargs=None,
        stdin=None,
        processed_parameters=None,
        **popen_options,
    ):
        """

        Passing --help to this function will not work.
        """
        if not self._session:
            # We create session and an empty XY project in one step.
            self._create_session()

        pack_importer_exporter = ImporterExporter(
            run_function=self.no_nonsense_run,
            run_cmd_function=self.no_nonsense_run_from_list,
        )
        command = pack_importer_exporter.process_parameter_list(command)

        if not self._crs_initialized:
            self._initialize_crs(pack_importer_exporter.input_rasters)

        pack_importer_exporter.import_data()

        if not self._is_region_modified() and pack_importer_exporter.input_rasters:
            # Reset the region for every run or keep it persistent?
            # Also, we now set that even for an existing session, this is
            # consistent with behavior without a provided session.
            # We could use env to pass the regions which would allow for the change
            # while not touching the underlying session.
            self.no_nonsense_run(
                "g.region",
                raster=pack_importer_exporter.input_rasters[0][0].stem,
                env=self._session.env,
            )
            self._region_is_set = True
        result = self.no_nonsense_run_from_list(command)
        pack_importer_exporter.export_data()
        return result

    def no_nonsense_run(self, name, /, *, tool_kwargs=None, stdin=None, **kwargs):
        args, popen_options = gs.popen_args_command(name, **kwargs)
        return self.no_nonsense_run_from_list(
            args, tool_kwargs=tool_kwargs, stdin=stdin, **popen_options
        )

    def no_nonsense_run_from_list(
        self, command, tool_kwargs=None, stdin=None, **popen_options
    ):
        if not self._session:
            # We create session and an empty XY project in one step.
            self._create_session()
        if not self._tools:
            self._tools = Tools(
                overwrite=True,
                quiet=False,
                verbose=False,
                superquiet=False,
                errors=self._errors,
                capture_output=self._capture_output,
                session=self._session,
            )
        return self._tools.call_cmd(command)

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
        self._region_file = project_path / "PERMANENT" / "WIND"
        self._region_modified_time = self._region_file.stat().st_mtime
        self._session = gs.setup.init(project_path)

    def _initialize_crs(self, rasters):
        # Get the mapset path
        mapset_path = self.no_nonsense_run(
            "g.gisenv", get="GISDBASE,LOCATION_NAME,MAPSET", sep="/"
        ).text
        mapset_path = Path(mapset_path)

        if rasters:
            with tarfile.TarFile(rasters[0][0]) as tar:
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

    def _is_region_modified(self):
        if self._region_is_set:
            return True
        if not self._region_file:
            if self._session:
                self._region_file = (
                    Path(
                        self.no_nonsense_run(
                            "g.gisenv", get="GISDBASE,LOCATION_NAME", sep="/"
                        ).text
                    )
                    / "PERMANENT"
                    / "WIND"
                )
                self._region_modified_time = self._region_file.stat().st_mtime
            return False
        return self._region_file.stat().st_mtime > self._region_modified_time

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

    def __getattr__(self, name):
        """Parse attribute to GRASS display module. Attribute should be in
        the form 'd_module_name'. For example, 'd.rast' is called with 'd_rast'.
        """
        if not self._session:
            # We create session and an empty XY project in one step.
            self._create_session()
        if not self._name_helper:
            self._name_helper = ToolFunctionResolver(
                run_function=self.run, env=self._session.env
            )
        return self._name_helper.get_function(name, exception_type=AttributeError)
