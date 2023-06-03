#!/usr/bin/env python

##############################################################################
# AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>
#
# PURPOSE:   API to call GRASS tools (modules) as Python functions
#
# COPYRIGHT: (C) 2023 Vaclav Petras and the GRASS Development Team
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


class ExecutedTool:
    """Result returned after executing a tool"""

    def __init__(self, name, kwargs, stdout, stderr):
        self._name = name
        self._kwargs = kwargs
        self._stdout = stdout
        self._stderr = stderr
        if self._stdout:
            self._decoded_stdout = gs.decode(self._stdout)
        else:
            self._decoded_stdout = ""

    @property
    def text(self) -> str:
        """Text output as decoded string"""
        return self._decoded_stdout.strip()

    @property
    def json(self):
        """Text output read as JSON

        This returns the nested structure of dictionaries and lists or fails when
        the output is not JSON.
        """
        return json.loads(self._stdout)

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

    # These could be public, not protected.
    def _freeze_region(self):
        self._env["GRASS_REGION"] = gs.region_env(env=self._env)
        self._region_is_frozen = True

    def _overwrite(self):
        self._env["GRASS_OVERWRITE"] = "1"

    def _set_stdin(self, stdin, /):
        print("_set_stdin", stdin)
        self._stdin = stdin

    @property
    def env(self):
        """Internally used environment (reference to it, not a copy)"""
        return self._env

    def run(self, name, /, **kwargs):
        """Run modules from the GRASS display family (modules starting with "d.").

         This function passes arguments directly to grass.script.run_command()
         so the syntax is the same.

        :param str module: name of GRASS module
        :param `**kwargs`: named arguments passed to run_command()"""
        # alternatively use dev null as default or provide it as convenient settings
        stdout_pipe = gs.PIPE
        stderr_pipe = gs.PIPE
        if self._stdin:
            stdin_pipe = gs.PIPE
            stdin = gs.utils.encode(self._stdin)
        else:
            stdin_pipe = None
            stdin = None
        process = gs.start_command(
            name,
            env=self._env,
            **kwargs,
            stdin=stdin_pipe,
            stdout=stdout_pipe,
            stderr=stderr_pipe,
        )
        stdout, stderr = process.communicate(input=stdin)
        stderr = gs.utils.decode(stderr)
        returncode = process.poll()
        if returncode and self._errors != "ignore":
            raise gs.CalledModuleError(
                name,
                code=" ".join([f"{key}={value}" for key, value in kwargs.items()]),
                returncode=returncode,
                errors=stderr,
            )
        return ExecutedTool(name=name, kwargs=kwargs, stdout=stdout, stderr=stderr)

    def feed_input_to(self, stdin, /):
        """Get a new object which will feed text input to a tool or tools"""
        return Tools(env=self._env, stdin=stdin)

    def ignore_errors_of(self):
        """Get a new object which will ignore errors of the called tools"""
        return Tools(env=self._env, errors="ignore")

    def __getattr__(self, name):
        """Parse attribute to GRASS display module. Attribute should be in
        the form 'd_module_name'. For example, 'd.rast' is called with 'd_rast'.
        """
        # Reformat string
        grass_module = name.replace("_", ".")
        # Assert module exists
        if not shutil.which(grass_module):
            raise AttributeError(
                _(
                    "Cannot find GRASS tool {}. "
                    "Is the session set up and the tool on path?"
                ).format(grass_module)
            )

        def wrapper(**kwargs):
            # Run module
            return self.run(grass_module, **kwargs)

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
    # gs.feed_command("v.in.ascii",
    #    input="-", output="point", separator=",",
    #    stdin="13.45,29.96,200", overwrite=True)
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

    # try:
    tools_pro.feed_input_to("13.45,29.96,200").v_in_ascii(
        input="-",
        output="point",
        format="xstandard",
    )
    # except gs.CalledModuleError as error:
    #    print("Exception text:")
    #    print(error)


if __name__ == "__main__":
    _test()
