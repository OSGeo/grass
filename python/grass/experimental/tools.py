#!/usr/bin/env python

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

"""API to call GRASS tools (modules) as Python functions"""

import json
from pathlib import Path

import grass.script as gs
import grass.tools
from grass.tools.support import ParameterConverter


class PackImporterExporter:
    def __init__(self, *, run_function, env=None):
        self._run_function = run_function
        self._env = env

    @classmethod
    def is_raster_pack_file(cls, value):
        return value.endswith((".grass_raster", ".pack", ".rpack", ".grr"))

    def modify_and_ingest_argument_list(self, args, parameters):
        # Uses parameters, but modifies the command, generates list of rasters and vectors.
        self.input_rasters = []
        if "inputs" in parameters:
            for item in parameters["inputs"]:
                if self.is_raster_pack_file(item["value"]):
                    self.input_rasters.append(Path(item["value"]))
                    # No need to change that for the original kwargs.
                    # kwargs[item["param"]] = Path(item["value"]).stem
                    # Actual parameters to execute are now a list.
                    for i, arg in enumerate(args):
                        if arg.startswith(f"{item['param']}="):
                            arg = arg.replace(item["value"], Path(item["value"]).stem)
                            args[i] = arg
        self.output_rasters = []
        if "outputs" in parameters:
            for item in parameters["outputs"]:
                if self.is_raster_pack_file(item["value"]):
                    self.output_rasters.append(Path(item["value"]))
                    # kwargs[item["param"]] = Path(item["value"]).stem
                    for i, arg in enumerate(args):
                        if arg.startswith(f"{item['param']}="):
                            arg = arg.replace(item["value"], Path(item["value"]).stem)
                            args[i] = arg

    def import_rasters(self):
        for raster_file in self.input_rasters:
            # Currently we override the projection check.
            self._run_function(
                "r.unpack",
                input=raster_file,
                output=raster_file.stem,
                overwrite=True,
                superquiet=True,
                # flags="o",
                env=self._env,
            )

    def export_rasters(self):
        # Pack the output raster
        for raster in self.output_rasters:
            # Overwriting a file is a warning, so to avoid it, we delete the file first.
            Path(raster).unlink(missing_ok=True)

            self._run_function(
                "r.pack",
                input=raster.stem,
                output=raster,
                flags="c",
                overwrite=True,
                superquiet=True,
            )

    def import_data(self):
        self.import_rasters()

    def export_data(self):
        self.export_rasters()


class Tools(grass.tools.Tools):
    """In addition to Tools, it processes arguments which are raster pack files"""

    def _process_parameters(self, command, **popen_options):
        popen_options["stdin"] = None
        popen_options["stdout"] = gs.PIPE
        # We respect whatever is in the stderr option because that's what the user
        # asked for and will expect to get in case of error (we pretend that it was
        # the intended run, not our special run before the actual run).
        return self.call_cmd([*command, "--json"], **popen_options)

    def run(self, name, /, **kwargs):
        object_parameter_handler = ParameterConverter()
        object_parameter_handler.process_parameters(kwargs)

        args, popen_options = gs.popen_args_command(name, **kwargs)

        interface_result = self._process_parameters(args, **popen_options)
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

        # We approximate tool_kwargs as original kwargs.
        return self.run_cmd(
            args,
            tool_kwargs=kwargs,
            processed_parameters=parameters,
            stdin=object_parameter_handler.stdin,
            **popen_options,
        )

    def run_cmd(
        self,
        command,
        tool_kwargs=None,
        stdin=None,
        processed_parameters=None,
        **popen_options,
    ):
        if not processed_parameters:
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
            stdin=stdin,
            **popen_options,
        )
        pack_importer_exporter.export_data()
        return result
