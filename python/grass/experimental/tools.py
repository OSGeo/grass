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

import grass.script as gs
from grass.exceptions import CalledModuleError


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
        # TODO: cache parsed JSON
        if self._stdout:
            # We are testing just std out and letting rest to the parse and the user.
            # This makes no assumption about how JSON is produced by the tool.
            return self.json[name]
        msg = f"Output of the tool {self._name} is not JSON"
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
        # Does not check for multiple set at the same time, but the most versbose wins
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

    def _digest_data_parameters(self, parameters, command):
        # Uses parameters, but modifies the command.
        input_rasters = []
        if "inputs" in parameters:
            for item in parameters["inputs"]:
                if item["value"].endswith(".grass_raster"):
                    input_rasters.append(Path(item["value"]))
                    for i, arg in enumerate(command):
                        if arg.startswith(f"{item['param']}="):
                            arg = arg.replace(item["value"], Path(item["value"]).stem)
                            command[i] = arg
        output_rasters = []
        if "outputs" in parameters:
            for item in parameters["outputs"]:
                if item["value"].endswith(".grass_raster"):
                    output_rasters.append(Path(item["value"]))
                    for i, arg in enumerate(command):
                        if arg.startswith(f"{item['param']}="):
                            arg = arg.replace(item["value"], Path(item["value"]).stem)
                            command[i] = arg
        return input_rasters, output_rasters


    def run(self, name, /, **kwargs):
        """Run modules from the GRASS display family (modules starting with "d.").

         This function passes arguments directly to grass.script.run_command()
         so the syntax is the same.

        :param str module: name of GRASS module
        :param `**kwargs`: named arguments passed to run_command()"""
        original = {}
        original_outputs = {}
        import grass.script.array as garray
        import numpy as np
        for key, value in kwargs.items():
            if isinstance(value, np.ndarray):
                kwargs[key] = "tmp_serialized_array"
                original[key] = value
            elif value == np.ndarray:
                kwargs[key] = "tmp_future_serialized_array"
                original_outputs[key] = value

        args, popen_options = gs.popen_args_command(name, **kwargs)

        env = popen_options.get("env", self._env)

        import subprocess
        parameters = json.loads(
            subprocess.check_output(
                [*args, "--json"], text=True, env=env
            )
        )
        if "inputs" in parameters:
            for param in parameters["inputs"]:
                if param["param"] not in original:
                    continue
                map2d = garray.array()
                print(param)
                map2d[:] = original[param["param"]]
                map2d.write("tmp_serialized_array", overwrite=True)

        # We approximate tool_kwargs as original kwargs.
        result = self.run_from_list(args, tool_kwargs=kwargs, **popen_options)

        if "outputs" in parameters:
            for param in parameters["outputs"]:
                if param["param"] not in original_outputs:
                    continue
                output_array = garray.array("tmp_future_serialized_array")
                result = output_array

        return result

    def run_command(self, name, /, **kwargs):
        # Adjust error handling or provide custom implementation for full control?
        return gs.run_command(name, **kwargs, env=self._env)

    def parse_command(self, name, /, **kwargs):
        return gs.parse_command(name, **kwargs, env=self._env)

    # Make this an overload of run.
    def run_from_list(self, command, tool_kwargs=None, **popen_options):
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
        return Tools(env=self._env, stdin=stdin)

    def ignore_errors_of(self):
        """Get a new object which will ignore errors of the called tools"""
        return Tools(env=self._env, errors="ignore")

    def levenshtein_distance(self, text1: str, text2: str) -> int:
        if len(text1) < len(text2):
            return self.levenshtein_distance(text2, text1)

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

    def suggest_tools(self, tool):
        # TODO: cache commands also for dir
        all_names = list(gs.get_commands()[0])
        result = []
        max_suggestions = 10
        for name in all_names:
            if self.levenshtein_distance(tool, name) < len(tool) / 2:
                result.append(name)
            if len(result) >= max_suggestions:
                break
        return result

    def __getattr__(self, name):
        """Parse attribute to GRASS display module. Attribute should be in
        the form 'd_module_name'. For example, 'd.rast' is called with 'd_rast'.
        """
        # Reformat string
        tool_name = name.replace("_", ".")
        # Assert module exists
        if not shutil.which(tool_name):
            suggesions = self.suggest_tools(tool_name)
            if suggesions:
                msg = (
                    f"Tool {tool_name} not found. "
                    f"Did you mean: {', '.join(suggesions)}?"
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
            return self.run(tool_name, **kwargs)

        return wrapper


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
