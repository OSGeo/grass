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
import os
import shutil
import subprocess
from pathlib import Path
from io import StringIO

import numpy as np

import grass.script as gs
import grass.script.array as garray
from grass.exceptions import CalledModuleError


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


class ObjectParameterHandler:
    def __init__(self):
        self._numpy_inputs = {}
        self._numpy_outputs = {}
        self._numpy_inputs_ordered = []
        self.stdin = None

    def process_parameters(self, kwargs):
        for key, value in kwargs.items():
            if isinstance(value, np.ndarray):
                kwargs[key] = gs.append_uuid("tmp_serialized_input_array")
                self._numpy_inputs[key] = value
                self._numpy_inputs_ordered.append(value)
            elif value in (np.ndarray, np.array, garray.array):
                # We test for class or the function.
                kwargs[key] = gs.append_uuid("tmp_serialized_output_array")
                self._numpy_outputs[key] = value
            elif isinstance(value, StringIO):
                kwargs[key] = "-"
                self.stdin = value.getvalue()

    def translate_objects_to_data(self, kwargs, parameters, env):
        if "inputs" in parameters:
            for param in parameters["inputs"]:
                if param["param"] in self._numpy_inputs:
                    map2d = garray.array(env=env)
                    map2d[:] = self._numpy_inputs[param["param"]]
                    map2d.write(kwargs[param["param"]])

    def input_rows_columns(self):
        if not len(self._numpy_inputs_ordered):
            return None
        return self._numpy_inputs_ordered[0].shape

    def translate_data_to_objects(self, kwargs, parameters, env):
        output_arrays = []
        if "outputs" in parameters:
            for param in parameters["outputs"]:
                if param["param"] not in self._numpy_outputs:
                    continue
                output_array = garray.array(kwargs[param["param"]], env=env)
                output_arrays.append(output_array)
        if len(output_arrays) == 1:
            self.result = output_arrays[0]
            return True
        if len(output_arrays) > 1:
            self.result = tuple(output_arrays)
            return True
        self.result = None
        return False


class ToolFunctionNameHelper:
    def __init__(self, *, run_function, env, prefix=None):
        self._run_function = run_function
        self._env = env
        self._prefix = prefix

    # def __getattr__(self, name):
    #    self.get_function(name, exception_type=AttributeError)

    def get_function(self, name, exception_type):
        """Parse attribute to GRASS display module. Attribute should be in
        the form 'd_module_name'. For example, 'd.rast' is called with 'd_rast'.
        """
        if self._prefix:
            name = f"{self._prefix}.{name}"
        # Reformat string
        tool_name = name.replace("_", ".")
        # Assert module exists
        if not shutil.which(tool_name, path=self._env["PATH"]):
            suggestions = self.suggest_tools(tool_name)
            if suggestions:
                msg = (
                    f"Tool {tool_name} not found. "
                    f"Did you mean: {', '.join(suggestions)}?"
                )
                raise AttributeError(msg)
            msg = (
                f"Tool or attribute {name} not found. "
                "If you are executing a tool, is the session set up and the tool on path? "
                "If you are looking for an attribute, is it in the documentation?"
            )
            raise AttributeError(msg)

        def wrapper(**kwargs):
            # Run module
            return self._run_function(tool_name, **kwargs)

        return wrapper

    @staticmethod
    def levenshtein_distance(text1: str, text2: str) -> int:
        if len(text1) < len(text2):
            return ToolFunctionNameHelper.levenshtein_distance(text2, text1)

        if len(text2) == 0:
            return len(text1)

        previous_row = list(range(len(text2) + 1))
        for i, char1 in enumerate(text1):
            current_row = [i + 1]
            for j, char2 in enumerate(text2):
                insertions = previous_row[j + 1] + 1
                deletions = current_row[j] + 1
                substitutions = previous_row[j] + (char1 != char2)
                current_row.append(min(insertions, deletions, substitutions))
            previous_row = current_row

        return previous_row[-1]

    @staticmethod
    def suggest_tools(tool):
        # TODO: cache commands also for dir
        all_names = list(gs.get_commands()[0])
        result = []
        max_suggestions = 10
        for name in all_names:
            if ToolFunctionNameHelper.levenshtein_distance(tool, name) < len(tool) / 2:
                result.append(name)
            if len(result) >= max_suggestions:
                break
        return result


class ExecutedTool:
    """Result returned after executing a tool"""

    def __init__(self, name, kwargs, stdout, stderr):
        self._name = name
        self._kwargs = kwargs
        self._stdout = stdout
        self._stderr = stderr
        if self._stdout is not None:
            self._decoded_stdout = gs.decode(self._stdout)
        else:
            self._decoded_stdout = None
        self._cached_json = None

    @property
    def text(self) -> str:
        """Text output as decoded string"""
        if self._decoded_stdout is None:
            return None
        return self._decoded_stdout.strip()

    @property
    def json(self):
        """Text output read as JSON

        This returns the nested structure of dictionaries and lists or fails when
        the output is not JSON.
        """
        if self._cached_json is None:
            self._cached_json = json.loads(self._stdout)
        return self._cached_json

    @property
    def keyval(self):
        """Text output read as key-value pairs separated by equal signs"""

        def conversion(value):
            """Convert text to int or float if possible, otherwise return it as is"""
            try:
                return int(value)
            except ValueError:
                pass
            try:
                return float(value)
            except ValueError:
                pass
            return value

        return gs.parse_key_val(self._stdout, val_type=conversion)

    @property
    def comma_items(self):
        """Text output read as comma-separated list"""
        return self.text_split(",")

    @property
    def space_items(self):
        """Text output read as whitespace-separated list"""
        return self.text_split(None)

    def text_split(self, separator=None):
        """Parse text output read as list separated by separators

        Any leading or trailing newlines are removed prior to parsing.
        """
        # The use of strip is assuming that the output is one line which
        # ends with a newline character which is for display only.
        return self._decoded_stdout.strip("\n").split(separator)

    def __getitem__(self, name):
        if self._stdout:
            # We are testing just std out and letting rest to the parse and the user.
            # This makes no assumption about how JSON is produced by the tool.
            try:
                return self.json[name]
            except json.JSONDecodeError as error:
                if self._kwargs.get("format") == "json":
                    raise
                msg = (
                    f"Output of {self._name} cannot be parsed as JSON. "
                    'Did you use format="json"?'
                )
                raise ValueError(msg) from error
        msg = f"No text output for {self._name} to be parsed as JSON"
        raise ValueError(msg)


class Tools:
    """Call GRASS tools as methods

    GRASS tools (modules) can be executed as methods of this class.
    """

    def __init__(
        self,
        *,
        session=None,
        env=None,
        overwrite=False,
        quiet=False,
        verbose=False,
        superquiet=False,
        freeze_region=False,
        stdin=None,
        errors=None,
        capture_output=True,
        prefix=None,
    ):
        if env:
            self._env = env.copy()
        elif session and hasattr(session, "env"):
            self._env = session.env.copy()
        else:
            self._env = os.environ.copy()
        self._region_is_frozen = False
        if freeze_region:
            self._freeze_region()
        if overwrite:
            self._overwrite()
        # This hopefully sets the numbers directly. An alternative implementation would
        # be to pass the parameter every time.
        # Does not check for multiple set at the same time, but the most verbose wins
        # for safety.
        if superquiet:
            self._env["GRASS_VERBOSE"] = "0"
        if quiet:
            self._env["GRASS_VERBOSE"] = "1"
        if verbose:
            self._env["GRASS_VERBOSE"] = "3"
        self._set_stdin(stdin)
        self._errors = errors
        self._capture_output = capture_output
        self._prefix = prefix
        self._name_helper = None

    # These could be public, not protected.
    def _freeze_region(self):
        self._env["GRASS_REGION"] = gs.region_env(env=self._env)
        self._region_is_frozen = True

    def _overwrite(self):
        self._env["GRASS_OVERWRITE"] = "1"

    def _set_stdin(self, stdin, /):
        self._stdin = stdin

    @property
    def env(self):
        """Internally used environment (reference to it, not a copy)"""
        return self._env

    def _process_parameters(self, command, popen_options):
        env = popen_options.get("env", self._env)

        return subprocess.run(
            [*command, "--json"], text=True, capture_output=True, env=env
        )

    def run(self, name, /, **kwargs):
        """Run modules from the GRASS display family (modules starting with "d.").

         This function passes arguments directly to grass.script.run_command()
         so the syntax is the same.

        :param str module: name of GRASS module
        :param `**kwargs`: named arguments passed to run_command()"""

        object_parameter_handler = ObjectParameterHandler()
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
        object_parameter_handler.translate_objects_to_data(
            kwargs, parameters, env=self._env
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
            kwargs, parameters, env=self._env
        )
        if use_objects:
            result = object_parameter_handler.result
        return result

    def run_from_list(
        self,
        command,
        tool_kwargs=None,
        stdin=None,
        processed_parameters=None,
        **popen_options,
    ):
        if not processed_parameters:
            interface_result = self._process_parameters(command, popen_options)
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

        pack_importer_exporter = PackImporterExporter(run_function=self.no_nonsense_run)
        pack_importer_exporter.modify_and_ingest_argument_list(
            command, processed_parameters
        )
        pack_importer_exporter.import_data()

        # We approximate tool_kwargs as original kwargs.
        result = self.no_nonsense_run_from_list(
            command,
            tool_kwargs=tool_kwargs,
            stdin=stdin,
            **popen_options,
        )
        pack_importer_exporter.export_data()
        return result

    def run_command(self, name, /, **kwargs):
        # TODO: Provide custom implementation for full control
        return gs.run_command(name, **kwargs, env=self._env)

    def parse_command(self, name, /, **kwargs):
        # TODO: Provide custom implementation for full control
        return gs.parse_command(name, **kwargs, env=self._env)

    def no_nonsense_run(self, name, /, *, tool_kwargs=None, stdin=None, **kwargs):
        args, popen_options = gs.popen_args_command(name, **kwargs)
        return self.no_nonsense_run_from_list(
            args, tool_kwargs=tool_kwargs, stdin=stdin, **popen_options
        )

    # Make this an overload of run.
    def no_nonsense_run_from_list(
        self, command, tool_kwargs=None, stdin=None, **popen_options
    ):
        # alternatively use dev null as default or provide it as convenient settings
        if self._capture_output:
            stdout_pipe = gs.PIPE
            stderr_pipe = gs.PIPE
        else:
            stdout_pipe = None
            stderr_pipe = None
        if self._stdin:
            stdin_pipe = gs.PIPE
            stdin = gs.utils.encode(self._stdin)
        elif stdin:
            stdin_pipe = gs.PIPE
            stdin = gs.utils.encode(stdin)
        else:
            stdin_pipe = None
            stdin = None
        # Allowing to overwrite env, but that's just to have maximum flexibility when
        # the session is actually set up, but it may be confusing.
        if "env" not in popen_options:
            popen_options["env"] = self._env
        process = gs.Popen(
            command,
            stdin=stdin_pipe,
            stdout=stdout_pipe,
            stderr=stderr_pipe,
            **popen_options,
        )
        stdout, stderr = process.communicate(input=stdin)
        if stderr:
            stderr = gs.utils.decode(stderr)
        returncode = process.poll()
        if returncode and self._errors != "ignore":
            raise CalledModuleError(
                command[0],
                code=" ".join(command),
                returncode=returncode,
                errors=stderr,
            )
        # TODO: solve tool_kwargs is None
        # We don't have the keyword arguments to pass to the resulting object.
        return ExecutedTool(
            name=command[0], kwargs=tool_kwargs, stdout=stdout, stderr=stderr
        )

    def feed_input_to(self, stdin, /):
        """Get a new object which will feed text input to a tool or tools"""
        return Tools(
            env=self._env,
            stdin=stdin,
            freeze_region=self._region_is_frozen,
            errors=self._errors,
            capture_output=self._capture_output,
            prefix=self._prefix,
        )

    def ignore_errors_of(self):
        """Get a new object which will ignore errors of the called tools"""
        return Tools(env=self._env, errors="ignore")

    def __getattr__(self, name):
        """Parse attribute to GRASS display module. Attribute should be in
        the form 'd_module_name'. For example, 'd.rast' is called with 'd_rast'.
        """
        if not self._name_helper:
            self._name_helper = ToolFunctionNameHelper(
                run_function=self.run,
                env=self.env,
                prefix=self._prefix,
            )
        return self._name_helper.get_function(name, exception_type=AttributeError)


def _test():
    """Ad-hoc tests and examples of the Tools class"""
    session = gs.setup.init("~/grassdata/nc_spm_08_grass7/user1")

    tools = Tools()
    tools.g_region(raster="elevation")
    tools.r_slope_aspect(elevation="elevation", slope="slope", overwrite=True)
    print(tools.r_univar(map="slope", flags="g").keyval)

    print(tools.v_info(map="bridges", flags="c").text)
    print(
        tools.v_db_univar(map="bridges", column="YEAR_BUILT", format="json").json[
            "statistics"
        ]["mean"]
    )

    print(tools.g_mapset(flags="p").text)
    print(tools.g_mapsets(flags="l").text_split())
    print(tools.g_mapsets(flags="l").space_items)
    print(tools.g_gisenv(get="GISDBASE,LOCATION_NAME,MAPSET", sep="comma").comma_items)

    print(tools.g_region(flags="g").keyval)

    env = os.environ.copy()
    env["GRASS_REGION"] = gs.region_env(res=250)
    coarse_computation = Tools(env=env)
    current_region = coarse_computation.g_region(flags="g").keyval
    print(current_region["ewres"], current_region["nsres"])
    coarse_computation.r_slope_aspect(
        elevation="elevation", slope="slope", flags="a", overwrite=True
    )
    print(coarse_computation.r_info(map="slope", flags="g").keyval)

    independent_computation = Tools(session=session, freeze_region=True)
    tools.g_region(res=500)  # we would do this for another computation elsewhere
    print(independent_computation.g_region(flags="g").keyval["ewres"])

    tools_pro = Tools(
        session=session, freeze_region=True, overwrite=True, superquiet=True
    )
    tools_pro.r_slope_aspect(elevation="elevation", slope="slope")
    tools_pro.feed_input_to("13.45,29.96,200").v_in_ascii(
        input="-", output="point", separator=","
    )
    print(tools_pro.v_info(map="point", flags="t").keyval["points"])

    print(tools_pro.ignore_errors_of().g_version(flags="rge").keyval)

    elevation = "elevation"
    exaggerated = "exaggerated"
    tools_pro.r_mapcalc(expression=f"{exaggerated} = 5 * {elevation}")
    tools_pro.feed_input_to(f"{exaggerated} = 5 * {elevation}").r_mapcalc(file="-")


if __name__ == "__main__":
    _test()
