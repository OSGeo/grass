import subprocess
from pathlib import Path


class ImporterExporter:
    raster_pack_suffixes = (".grass_raster", ".pack", ".rpack", ".grr")

    @classmethod
    def is_recognized_file(cls, value):
        return cls.is_raster_pack_file(value)

    @classmethod
    def is_raster_pack_file(cls, value):
        if isinstance(value, str):
            return value.endswith(cls.raster_pack_suffixes)
        if isinstance(value, Path):
            return value.suffix in cls.raster_pack_suffixes
        return False

    def __init__(self, *, run_function, run_cmd_function):
        self._run_function = run_function
        self._run_cmd_function = run_cmd_function
        self.input_rasters: list[tuple] = []
        self.output_rasters: list[tuple] = []

    def process_parameter_list(self, command, **popen_options):
        """Ingests any file for later imports and exports and replaces arguments

        This function is relatively costly as it calls a subprocess to digest the parameters.

        Returns the list of parameters with inputs and outputs replaced so that a tool
        will understand that, i.e., file paths into data names in a project.
        """
        # Get processed parameters to distinguish inputs and outputs.
        parameters = self._process_parameters(command, **popen_options)
        tool_name = parameters["module"]
        args = command.copy()
        if "inputs" in parameters:
            for item in parameters["inputs"]:
                if tool_name != "r.unpack" and self.is_raster_pack_file(item["value"]):
                    in_project_name = self._to_name(item["value"])
                    self.input_rasters.append((Path(item["value"]), in_project_name))
                    for i, arg in enumerate(args):
                        if arg.startswith(f"{item['param']}="):
                            arg = arg.replace(item["value"], in_project_name)
                            args[i] = arg
        if "outputs" in parameters:
            for item in parameters["outputs"]:
                if tool_name != "r.pack" and self.is_raster_pack_file(item["value"]):
                    in_project_name = self._to_name(item["value"])
                    self.output_rasters.append((Path(item["value"]), in_project_name))
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

    def import_rasters(self):
        for raster_file, in_project_name in self.input_rasters:
            self._run_function(
                "r.unpack",
                input=raster_file,
                output=in_project_name,
                overwrite=True,
                superquiet=True,
            )

    def export_rasters(self):
        # Pack the output raster
        for raster_file, in_project_name in self.output_rasters:
            # Overwriting a file is a warning, so to avoid it, we delete the file first.
            # This creates a behavior consistent with command line tools.
            Path(raster_file).unlink(missing_ok=True)

            self._run_function(
                "r.pack",
                input=in_project_name,
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
