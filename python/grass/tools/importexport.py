from __future__ import annotations

import subprocess
from pathlib import Path
from typing import Literal


class ImporterExporter:
    """Imports and exports data while keeping track of it

    This is a class for internal use, but it may mature into a generally useful tool.
    """

    raster_pack_suffixes = (".grass_raster", ".pack", ".rpack", ".grr")

    @classmethod
    def is_recognized_file(cls, value):
        """Return `True` if file type is a recognized type, `False` otherwise"""
        return cls.is_raster_pack_file(value)

    @classmethod
    def is_raster_pack_file(cls, value):
        """Return `True` if file type is GRASS raster pack, `False` otherwise"""
        if isinstance(value, str):
            return value.endswith(cls.raster_pack_suffixes)
        if isinstance(value, Path):
            return value.suffix in cls.raster_pack_suffixes
        return False

    def __init__(self, *, run_function, run_cmd_function):
        self._run_function = run_function
        self._run_cmd_function = run_cmd_function
        # At least for reading purposes, public access to the lists makes sense.
        self.input_rasters: list[tuple[Path, str]] = []
        self.output_rasters: list[tuple[Path, str]] = []
        self.current_input_rasters: list[tuple[Path, str]] = []
        self.current_output_rasters: list[tuple[Path, str]] = []

    def process_parameter_list(self, command, **popen_options):
        """Ingests any file for later imports and exports and replaces arguments

        This function is relatively costly as it calls a subprocess to digest the parameters.

        Returns the list of parameters with inputs and outputs replaced so that a tool
        will understand that, i.e., file paths into data names in a project.
        """
        # Get processed parameters to distinguish inputs and outputs.
        # We actually don't know the type of the input or outputs) because that is
        # currently not included in --json. Consequently, we are only assuming that the
        # files are meant to be used as in-project data. So, we need to deal with cases
        # where that's not true one by one, such as r.unpack taking file,
        # not raster (cell), so the file needs to be left as is.
        parameters = self._process_parameters(command, **popen_options)
        tool_name = parameters["module"]
        args = command.copy()
        # We will deal with inputs right away
        if "inputs" in parameters:
            for item in parameters["inputs"]:
                if tool_name != "r.unpack" and self.is_raster_pack_file(item["value"]):
                    in_project_name = self._to_name(item["value"])
                    record = (Path(item["value"]), in_project_name)
                    if (
                        record not in self.output_rasters
                        and record not in self.input_rasters
                        and record not in self.current_input_rasters
                    ):
                        self.current_input_rasters.append(record)
                    for i, arg in enumerate(args):
                        if arg.startswith(f"{item['param']}="):
                            arg = arg.replace(item["value"], in_project_name)
                            args[i] = arg
        if "outputs" in parameters:
            for item in parameters["outputs"]:
                if tool_name != "r.pack" and self.is_raster_pack_file(item["value"]):
                    in_project_name = self._to_name(item["value"])
                    record = (Path(item["value"]), in_project_name)
                    # Following the logic of r.slope.aspect, we don't deal with one output repeated
                    # more than once, but this would be the place to address it.
                    if (
                        record not in self.output_rasters
                        and record not in self.current_output_rasters
                    ):
                        self.current_output_rasters.append(record)
                    for i, arg in enumerate(args):
                        if arg.startswith(f"{item['param']}="):
                            arg = arg.replace(item["value"], in_project_name)
                            args[i] = arg
        return args

    def _process_parameters(self, command, **popen_options):
        """Get parameters processed by the tool itself"""
        popen_options["stdin"] = None
        popen_options["stdout"] = subprocess.PIPE
        # We respect whatever is in the stderr option because that's what the user
        # asked for and will expect to get in case of error (we pretend that it was
        # the intended run, not our special run before the actual run).
        return self._run_cmd_function([*command, "--json"], **popen_options)

    def _to_name(self, value, /):
        return Path(value).stem

    def import_rasters(self, rasters, *, env):
        for raster_file, in_project_name in rasters:
            # Overwriting here is driven by the run function.
            self._run_function(
                "r.unpack",
                input=raster_file,
                output=in_project_name,
                superquiet=True,
                env=env,
            )

    def export_rasters(
        self, rasters, *, env, delete_first: bool, overwrite: Literal[True] | None
    ):
        # Pack the output raster
        for raster_file, in_project_name in rasters:
            # Overwriting a file is a warning, so to avoid it, we delete the file first.
            # This creates a behavior consistent with command line tools.
            if delete_first:
                Path(raster_file).unlink(missing_ok=True)

            # Overwriting here is driven by the run function and env.
            self._run_function(
                "r.pack",
                input=in_project_name,
                output=raster_file,
                flags="c",
                superquiet=True,
                env=env,
                overwrite=overwrite,
            )

    def import_data(self, *, env):
        # We import the data, make records for later, and the clear the current list.
        self.import_rasters(self.current_input_rasters, env=env)
        self.input_rasters.extend(self.current_input_rasters)
        self.current_input_rasters = []

    def export_data(
        self, *, env, delete_first: bool = False, overwrite: Literal[True] | None = None
    ):
        # We export the data, make records for later, and the clear the current list.
        self.export_rasters(
            self.current_output_rasters,
            env=env,
            delete_first=delete_first,
            overwrite=overwrite,
        )
        self.output_rasters.extend(self.current_output_rasters)
        self.current_output_rasters = []

    def cleanup(self, *, env):
        # We don't track in what mapset the rasters are, and we assume
        # the mapset was not changed in the meantime.
        remove = [name for (unused, name) in self.input_rasters]
        remove.extend([name for (unused, name) in self.output_rasters])
        if remove:
            self._run_function(
                "g.remove",
                type="raster",
                name=remove,
                superquiet=True,
                flags="f",
                env=env,
            )
        self.input_rasters = []
        self.output_rasters = []
