##############################################################################
# AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>
#
# PURPOSE:   API to call GRASS tools (modules) as Python functions
#
# COPYRIGHT: (C) 2023-2025 Vaclav Petras and the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.
##############################################################################

"""The module provides an API to use GRASS tools (modules) as Python functions"""

from __future__ import annotations

import json
from pathlib import Path

import grass.script as gs
import grass.tools
from grass.tools.support import ParameterConverter


class PackImporterExporter:
    def __init__(self, *, run_function, env=None):
        self._run_function = run_function
        self._env = env
        self.input_rasters: list[tuple] = []
        self.output_rasters: list[tuple] = []

    @classmethod
    def is_raster_pack_file(cls, value):
        return value.endswith((".grass_raster", ".pack", ".rpack", ".grr"))

    def modify_and_ingest_argument_list(self, args, parameters):
        # TODO: Deal with r.pack and r.unpack calls.
        # Uses parameters, but modifies the command, generates list of rasters and vectors.
        if "inputs" in parameters:
            for item in parameters["inputs"]:
                if self.is_raster_pack_file(item["value"]):
                    inproject_name = Path(item["value"]).stem
                    self.input_rasters.append((Path(item["value"]), inproject_name))
                    # No need to change that for the original kwargs.
                    # kwargs[item["param"]] = Path(item["value"]).stem
                    # Actual parameters to execute are now a list.
                    for i, arg in enumerate(args):
                        if arg.startswith(f"{item['param']}="):
                            arg = arg.replace(item["value"], inproject_name)
                            args[i] = arg
        if "outputs" in parameters:
            for item in parameters["outputs"]:
                if self.is_raster_pack_file(item["value"]):
                    inproject_name = Path(item["value"]).stem
                    self.output_rasters.append((Path(item["value"]), inproject_name))
                    # kwargs[item["param"]] = Path(item["value"]).stem
                    for i, arg in enumerate(args):
                        if arg.startswith(f"{item['param']}="):
                            arg = arg.replace(item["value"], inproject_name)
                            args[i] = arg

    def import_rasters(self):
        for raster_file, inproject_name in self.input_rasters:
            # Currently we override the projection check.
            self._run_function(
                "r.unpack",
                input=raster_file,
                output=inproject_name,
                overwrite=True,
                superquiet=True,
                # flags="o",
                env=self._env,
            )

    def export_rasters(self):
        # Pack the output raster
        for raster_file, inproject_name in self.output_rasters:
            # Overwriting a file is a warning, so to avoid it, we delete the file first.
            Path(raster_file).unlink(missing_ok=True)

            self._run_function(
                "r.pack",
                input=inproject_name,
                output=raster_file,
                flags="c",
                overwrite=True,
                superquiet=True,
            )

    def import_data(self):
        self.import_rasters()

    def export_data(self):
        self.export_rasters()

    def cleanup(self):
        remove = [name for (unused, name) in self.input_rasters]
        remove.extend([name for (unused, name) in self.output_rasters])
        if remove:
            self._run_function(
                "g.remove", type="raster", name=remove, superquiet=True, flags="f"
            )


class Tools(grass.tools.Tools):
    """In addition to Tools, it processes arguments which are raster pack files"""

    def __init__(self, keep_data=None, **kwargs):
        super().__init__(**kwargs)
        self._delete_on_context_exit = False
        self._keep_data = keep_data
        self._cleanups = []

    def _process_parameters(self, command, **popen_options):
        popen_options["stdin"] = None
        popen_options["stdout"] = gs.PIPE
        # We respect whatever is in the stderr option because that's what the user
        # asked for and will expect to get in case of error (we pretend that it was
        # the intended run, not our special run before the actual run).
        return self.call_cmd([*command, "--json"], **popen_options)

    def run(self, tool_name_: str, /, **kwargs):
        """Run a tool by specifying its name as a string and parameters.

        The parameters tool are tool name as a string and parameters as keyword
        arguments. The keyword arguments may include an argument *flags* which is a
        string of one-character tool flags.

        The function may perform additional processing on the parameters.

        :param tool_name_: name of a GRASS tool
        :param kwargs: tool parameters
        """
        # Object parameters are handled first before the conversion of the call to a
        # list of strings happens.
        object_parameter_handler = ParameterConverter()
        object_parameter_handler.process_parameters(kwargs)

        # Get a fixed env parameter at at the beginning of each execution,
        # but repeat it every time in case the referenced environment is modified.
        args, popen_options = gs.popen_args_command(tool_name_, **kwargs)
        # We approximate original kwargs with the possibly-modified kwargs.
        return self.run_cmd(
            args,
            tool_kwargs=kwargs,
            input=object_parameter_handler.stdin,
            **popen_options,
        )

    def run_cmd(
        self,
        command: list[str],
        *,
        input: str | bytes | None = None,
        tool_kwargs: dict | None = None,
        **popen_options,
    ):
        """Run a tool by passing its name and parameters a list of strings.

        The function may perform additional processing on the parameters.

        :param command: list of strings to execute as the command
        :param input: text input for the standard input of the tool
        :param tool_kwargs: named tool arguments used for error reporting (experimental)
        :param **popen_options: additional options for :py:func:`subprocess.Popen`
        """
        interface_result = self._process_parameters(command, **popen_options)
        if interface_result.returncode != 0:
            # This is only for the error states.
            return gs.handle_errors(
                interface_result.returncode,
                result=None,
                args=[command],
                kwargs=tool_kwargs,
                stderr=interface_result.stderr,
                handler="raise",
            )
        processed_parameters = json.loads(interface_result.stdout)

        pack_importer_exporter = PackImporterExporter(run_function=self.call)
        pack_importer_exporter.modify_and_ingest_argument_list(
            command, processed_parameters
        )
        pack_importer_exporter.import_data()

        # We approximate tool_kwargs as original kwargs.
        result = self.call_cmd(
            command,
            tool_kwargs=tool_kwargs,
            input=input,
            **popen_options,
        )
        pack_importer_exporter.export_data()
        if self._delete_on_context_exit or self._keep_data:
            self._cleanups.append(pack_importer_exporter.cleanup)
        else:
            pack_importer_exporter.cleanup()
        return result

    def __enter__(self):
        """Enter the context manager context.

        :returns: reference to the object (self)
        """
        self._delete_on_context_exit = True
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        """Exit the context manager context."""
        if not self._keep_data:
            self.cleanup()

    def cleanup(self):
        for cleanup in self._cleanups:
            cleanup()
