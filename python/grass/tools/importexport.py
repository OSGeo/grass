from pathlib import Path


class PackImporterExporter:
    raster_pack_suffixes = (".grass_raster", ".pack", ".rpack", ".grr")

    @classmethod
    def is_recognized_file(cls, value):
        return cls.is_raster_pack_file(value)

    @classmethod
    def is_raster_pack_file(cls, value):
        if isinstance(value, (str, bytes)):
            return value.endswith(cls.raster_pack_suffixes)
        if isinstance(value, Path):
            return value.suffix in cls.raster_pack_suffixes
        return False

    def __init__(self, *, run_function, env=None):
        self._run_function = run_function
        self.input_rasters: list[tuple] = []
        self.output_rasters: list[tuple] = []

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
            self._run_function(
                "r.unpack",
                input=raster_file,
                output=inproject_name,
                overwrite=True,
                superquiet=True,
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
