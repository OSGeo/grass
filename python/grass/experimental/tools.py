import os
import sys
import shutil

import grass.script as gs


class ExecutedTool:
    def __init__(self, name, kwargs, stdout, stderr):
        self._name = name
        self._stdout = stdout
        self._decoded_stdout = gs.decode(self._stdout)

    @property
    def text(self):
        return self._decoded_stdout.strip()

    @property
    def json(self):
        import json

        return json.loads(self._stdout)

    @property
    def keyval(self):
        # TODO: possibly use or add _text_to_key_value_dict
        # which converts int and float automatically
        return gs.parse_key_val(self._stdout)

    @property
    def comma_items(self):
        return self.text_split(",")

    @property
    def space_items(self):
        return self.text_split(None)

    def text_split(self, separator=None):
        # The use of strip is assuming that the output is one line which
        # ends with a newline character which is for display only.
        return self._decoded_stdout.strip("\n").split(separator)


class SubExecutor:
    """use as tools().params(a="x", b="y").g_region()"""

    # a and b would be overwrite or stdin

    # Can support other envs or all PIPE and encoding read command supports


class Tools:
    def __init__(
        self,
        *,
        session=None,
        env=None,
        overwrite=True,
        quiet=False,
        verbose=False,
        superquiet=False,
        freeze_region=False,
    ):
        # TODO: fix region, so that external g.region call in the middle
        # is not a problem
        # i.e. region is independent/internal/fixed
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

    # These could be public, not protected.
    def _freeze_region(self):
        self._env["GRASS_REGION"] = gs.region_env(env=self._env)
        self._region_is_frozen = True

    def _overwrite(self):
        self._env["GRASS_OVERWRITE"] = "1"

    @property
    def env(self):
        return self._env

    def run(self, name, /, **kwargs):
        """Run modules from the GRASS display family (modules starting with "d.").

         This function passes arguments directly to grass.script.run_command()
         so the syntax is the same.

        :param str module: name of GRASS module
        :param `**kwargs`: named arguments passed to run_command()"""
        # alternatively use dev null as default or provide it as convenient settings
        kwargs["stdout"] = gs.PIPE
        kwargs["stderr"] = gs.PIPE
        process = gs.pipe_command(name, env=self._env, **kwargs)
        stdout, stderr = process.communicate()
        stderr = gs.utils.decode(stderr)
        returncode = process.poll()
        # TODO: instead of printing, do exception right away
        # but right now, handle errors does not accept stderr
        # or don't use handle errors and raise instead
        if returncode:
            raise gs.CalledModuleError(
                name,
                code=" ".join([f"{key}={value}" for key, value in kwargs.items()]),
                returncode=returncode,
                errors=stderr,
            )

            # Print only when we are capturing it and there was some output.
            # (User can request ignoring the subprocess stderr and then
            # we get only None.)
            if stderr:
                sys.stderr.write(stderr)
            gs.handle_errors(returncode, stdout, [name], kwargs)
        return ExecutedTool(name=name, kwargs=kwargs, stdout=stdout, stderr=stderr)

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
    print(
        current_region["ewres"], current_region["nsres"]
    )  # TODO: should keyval convert?
    coarse_computation.r_slope_aspect(
        elevation="elevation", slope="slope", flags="a", overwrite=True
    )
    print(coarse_computation.r_info(map="slope", flags="g").keyval)

    independent_computation = Tools(session=session, freeze_region=True)
    tools.g_region(res=500)  # we would do this for another computation elsewhere
    print(independent_computation.g_region(flags="g").keyval["ewres"])

    tools_pro = Tools(session=session, freeze_region=True, superquiet=True)
    tools_pro.r_slope_aspect(elevation="elevation", slope="slope")


if __name__ == "__main__":
    _test()
