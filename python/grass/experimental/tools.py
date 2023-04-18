import sys
import shutil

import grass.script as gs


class ExecutedTool:
    def __init__(self, name, kwargs, stdout, stderr):
        self._name = name
        self._stdout = stdout

    @property
    def text(self):
        return self._stdout

    @property
    def json(self):
        import json

        return json.loads(self._stdout)

    @property
    def keyval(self):
        return gs.parse_key_val(self._stdout)


class SubExecutor:
    """use as tools().params(a="x", b="y").g_region()"""

    # Can support other envs or all PIPE and encoding read command supports


class Tools:
    def __init__(self):
        # TODO: fix region, so that external g.region call in the middle
        # is not a problem
        # i.e. region is independent/internal/fixed
        pass

    def run(self, name, /, **kwargs):
        """Run modules from the GRASS display family (modules starting with "d.").

         This function passes arguments directly to grass.script.run_command()
         so the syntax is the same.

        :param str module: name of GRASS module
        :param `**kwargs`: named arguments passed to run_command()"""
        # alternatively use dev null as default or provide it as convenient settings
        kwargs["stdout"] = gs.PIPE
        kwargs["stderr"] = gs.PIPE
        process = gs.pipe_command(name, **kwargs)
        stdout, stderr = process.communicate()
        stderr = gs.utils.decode(stderr)
        returncode = process.poll()
        # TODO: instead of printing, do exception right away
        if returncode:
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
    gs.setup.init("~/grassdata/nc_spm_08_grass7/user1")

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


if __name__ == "__main__":
    _test()
