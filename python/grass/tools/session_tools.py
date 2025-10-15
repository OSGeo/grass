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

import os

import grass.script as gs
from grass.exceptions import CalledModuleError

from .importexport import ImporterExporter
from .support import ParameterConverter, ToolFunctionResolver, ToolResult


class ToolError(CalledModuleError):
    """Raised when a tool run ends with error (typically a non-zero return code)

    Inherits from *subprocess.CalledProcessError* to make it easy to transition from
    or combine with code which is using the *subprocess* package.
    Inherits from CalledModuleError to make it easy to transition code with except
    statements around *grass.script.run_command*-style tool calls, but new code should
    not rely on that.
    """

    def __init__(
        self, tool: str, cmd: list, returncode: int, errors: str | None = None
    ):
        """Create an exception with a full error message based on the parameters.

        Best results are provided when *errors* is a single line string, aiming at a
        short and clear message. In case *errors* is `None`, additional text is
        provided to help the user find the error message, assuming that there is one
        somewhere. If *errors* is an empty string, it assumes that stderr was produced
        and reports that in the resulting message. A single line message may be
        modified to provide the best possible error message assuming it is a standard
        fatal error message produced by a GRASS tool describing best what went wrong.
        For multiline error messages, no assumptions are made, so the information
        about the tool run is printed first and then the error message.

        :param tool: tool name (for interface compatibility with *CalledModuleError*)
        :param cmd: string or list of strings forming the actual (underlying) command
        :param returncode: process returncode (assuming non-zero)
        :param errors: errors provided by the tool (typically stderr)

        Expect changes to the *tool* parameter and the corresponding attribute.
        """
        # CalledProcessError has undocumented constructor
        super().__init__(tool, cmd, returncode, errors)
        errors_first = False
        # If provided, we assume errors are the actual tool errors with details, i.e.,
        # the captured stderr of the tool.
        if errors is None:
            # If the stderr was passed to the caller process instead of being capured
            # by the subprocess caller function, the stderr will be above
            # the traceback in the command line, but in notebooks or when testing,
            # the stderr will be somewhere else than the traceback.
            errors = "See errors above the traceback or in the error output (stderr)."
        elif errors == "":
            errors = "No error output was produced"
        elif "\n" not in errors.strip():
            errors_first = True
            # Remove end of line if any (and all other extra whitespace) and remove
            # error prefix in English.
            errors = errors.strip().removeprefix("ERROR: ")
        # Short, one line error message from stderr makes a good error message as the
        # first line of the exception text (shown by itself e.g. by pytest).
        # When not clear what is in, include the run details first and the potentially
        # long stderr afterwards.
        # While return code would be semantically better with a colon, there is already
        # a lot of colons in the resulting message (one after the exception type,
        # one likely from stderr, and another one depending on the order), so no colon
        # might be slightly easier to read.
        if errors_first:
            self.msg = (
                f"{errors}\nRun `{cmd}` ended with an error (return code {returncode})"
            )
        else:
            self.msg = (
                f"Run `{cmd}` ended with an error (return code {returncode}):\n{errors}"
            )
        self.tool = tool

    def __reduce__(self):
        return (
            self.__class__,
            (self.tool, self.cmd, self.returncode, self.errors),
        )

    def __str__(self):
        return self.msg


class Tools:
    """Use GRASS tools through function calls (experimental)

    GRASS tools (modules) can be executed as methods of this class.
    This API is experimental in version 8.5 and is expected to be stable in version 8.6.

    The tools can be used in an active GRASS session (this skipped when writing
    a GRASS tool):

    >>> import grass.script as gs
    >>> gs.create_project("xy_project")
    >>> session = gs.setup.init("xy_project")

    Multiple tools can be accessed through a single *Tools* object:

    >>> from grass.tools import Tools
    >>> tools = Tools(session=session)
    >>> tools.g_region(rows=100, cols=100)
    >>> tools.r_random_surface(output="surface", seed=42)

    For tools outputting JSON, the results can be accessed directly:

    >>> print("cells:", tools.g_region(flags="p", format="json")["cells"])
    cells: 10000

    Resulting text or other output can be accessed through attributes of
    the *ToolResult* object:

    >>> tools.g_region(flags="p").text  # doctest: +SKIP

    Text inputs, when a tool supports standard input (stdin), can be passed as *io.StringIO* objects:

    >>> from io import StringIO
    >>> tools.v_in_ascii(
    ...     input=StringIO("13.45,29.96,200"), output="point", separator=","
    ... )

    The *Tools* object can be used as a context manager:

    >>> with Tools(session=session) as tools:
    ...     tools.g_region(rows=100, cols=100)

    A tool can be accessed via a function with the same name as the tool.
    Alternatively, it can be called through one of the *run* or *call* functions.
    The *run* function provides convenient functionality for handling tool parameters,
    while the *call* function simply executes the tool. Both take tool parameters as
    keyword arguments. Each function has a corresponding variant which accepts a list
    of strings as parameters (*run_cmd* and *call_cmd*).
    When a tool is run using the function corresponding to its name, the *run* function
    is used in the background.

    Raster input and outputs can be NumPy arrays:

    >>> import numpy as np
    >>> tools.g_region(rows=2, cols=3)
    >>> slope = tools.r_slope_aspect(elevation=np.ones((2, 3)), slope=np.ndarray)
    >>> tools.r_grow(
    ...     input=np.array([[1, np.nan, np.nan], [np.nan, np.nan, np.nan]]),
    ...     radius=1.5,
    ...     output=np.ndarray,
    ... )
    array([[1., 1., 0.],
           [1., 1., 0.]])

    The input array's shape and the computational region rows and columns need to
    match. The output array's shape is determined by the computational region.

    When multiple outputs are returned, they are returned as a tuple:

    >>> (slope, aspect) = tools.r_slope_aspect(
    ...     elevation=np.ones((2, 3)), slope=np.array, aspect=np.array
    ... )

    To access the arrays by name, e.g., with a high number of output arrays,
    the standard result object can be requested with *consistent_return_value*:

    >>> tools = Tools(session=session, consistent_return_value=True)
    >>> result = tools.r_slope_aspect(
    ...     elevation=np.ones((2, 3)), slope=np.array, aspect=np.array
    ... )

    The result object than includes the arrays under the *arrays* attribute
    where they can be accessed as attributes by names corresponding to the
    output parameter names:

    >>> slope = result.arrays.slope
    >>> aspect = result.arrays.aspect

    Using `consistent_return_value=True` is also advantageous to obtain both arrays
    and text outputs from the tool as the result object has the same
    attributes and functionality as without arrays:

    >>> result.text
    ''

    Although using arrays incurs an overhead cost compared to using only
    in-project data, the array interface provides a convenient workflow
    when NumPy arrays are used with other array functions.

    If a tool accepts a single raster input or output, a native GRASS raster pack
    format can be used in the same way as an in-project raster or NumPy array.
    GRASS native rasters are recognized by `.grass_raster`, `.grr`, and `.rpack`
    extensions. All approaches can be combined in one workflow:

    >>> with Tools(session=session) as tools:
    ...     tools.r_slope_aspect(
    ...         elevation=np.ones((2, 3)), slope="slope.grass_raster", aspect="aspect"
    ...     )
    ...     statistics = tools.r_univar(map="slope.grass_raster", format="json")
    >>> # File now exists
    >>> from pathlib import Path
    >>> Path("slope.grass_raster").is_file()
    True
    >>> # In-project raster now exists
    >>> tools.r_info(map="aspect", format="json")["cells"]
    6

    When the *Tools* object is used as a context manager, in-project data created as
    part of handling the raster files will be cached and will not be imported again
    when used in the following steps. The cache is cleared at the end of the context.
    When the *Tools* object is not used as a context manager, the cashing can be
    enabled by `use_cache=True`. Explicitly enabled cache requires explicit cleanup:

    >>> tools = Tools(session=session, use_cache=True)
    >>> tools.r_univar(map="slope.grass_raster", format="json")["cells"]
    6
    >>> tools.r_info(map="slope.grass_raster", format="json")["cells"]
    6
    >>> tools.cleanup()

    Notably, the above code works also with `use_cache=False` (or the default),
    but the file will be imported twice, once for each tool call, so using
    context manager or managing the cache explicitly is good for reducing the
    overhead which the external rasters bring compared to using in-project data.

    For parallel processing, create separate Tools objects. Each Tools instance
    can operate with the same or different sessions or environments, as well as with
    :py:class:`grass.script.RegionManager` and :py:class:`grass.script.MaskManager`.
    When working exclusively with data within a project, objects are lightweight
    and add negligible overhead compared to direct subprocess calls.
    Using NumPy or out-of-project native GRASS raster files, adds computational
    and IO cost, but generally not more than the cost of the same operation done
    directly without the aid of a Tools object.
    """

    def __init__(
        self,
        *,
        session=None,
        env=None,
        overwrite=None,
        verbose=None,
        quiet=None,
        superquiet=None,
        errors=None,
        capture_output=True,
        capture_stderr=None,
        consistent_return_value=False,
        use_cache=None,
    ):
        """
        If session is provided and has an env attribute, it is used to execute tools.
        If env is provided, it is used to execute tools. If both session and env are
        provided, env is used to execute tools and session is ignored.
        However, session and env interaction may change in the future.

        If overwrite is provided, a an overwrite is set for all the tools.
        When overwrite is set to `False`, individual tool calls can set overwrite
        to `True`. If overwrite is set in the session or env, it is used.
        Note that once overwrite is set to `True` globally, an individual tool call
        cannot set it back to `False`.

        If verbose, quiet, superquiet is set to `True`, the corresponding verbosity level
        is set for all the tools. If one of them is set to `False` and the environment
        has the corresponding variable set, it is unset.
        The values cannot be combined. If multiple ones are set to `True`, the most
        verbose one wins.

        In case a tool run fails, indicating that by non-zero return code,
        *grass.tools.ToolError* exception is raised by default. This can
        be changed by passing, e.g., `errors="ignore"`. The *errors* parameter
        is passed to the *grass.script.handle_errors* function which determines
        the specific behavior.

        Text outputs from the tool are captured by default, both standard output
        (stdout) and standard error output (stderr). Both will be part of the result
        object returned by each tool run. Additionally, the standard error output will
        be included in the exception message. When *capture_output* is set to `False`,
        outputs are not captured in Python as values and go where the Python process
        outputs go (this is usually clear in command line, but less clear in a Jupyter
        notebook). When *capture_stderr* is set to `True`, the standard error output
        is captured and included in the exception message even if *capture_outputs*
        is set to `False`.

        A tool call will return a result object if the tool produces standard output
        (stdout) and `None` otherwise. If *consistent_return_value* is set to `True`,
        a call will return a result object even without standard output (*stdout* and
        *text* attributes of the result object will evaluate to `False`). This is
        advantageous when examining the *stdout* or *text* attributes directly, or
        when using the *returncode* attribute in combination with `errors="ignore"`.
        Additionally, this can be used to obtain both NumPy arrays and text outputs
        from a tool call.

        While using of cache is primarily driven by the use of the object as
        a context manager, cashing can be explicitly enabled or disabled with
        the *use_cache* parameter. The cached data is kept in the current
        mapset so that it is available as tool inputs. Without a context manager,
        explicit `use_cache=True` requires explicit call to *cleanup* to remove
        the data from the current mapset.

        If *env* or other *Popen* arguments are provided to one of the tool running
        functions, the constructor parameters except *errors* are ignored.
        """
        if env:
            self._original_env = env
        elif session and hasattr(session, "env"):
            self._original_env = session.env
        else:
            self._original_env = os.environ
        self._modified_env = None
        self._overwrite = overwrite
        self._verbose = verbose
        self._quiet = quiet
        self._superquiet = superquiet
        self._errors = errors
        self._capture_output = capture_output
        if capture_stderr is None:
            self._capture_stderr = capture_output
        else:
            self._capture_stderr = capture_stderr
        self._name_resolver = None
        self._consistent_return_value = consistent_return_value
        self._importer_exporter = None
        # Decides if we delete at each run or only at the end of context.
        self._delete_on_context_exit = False
        # User request to keep the data.
        self._use_cache = use_cache

    def _modified_env_if_needed(self):
        """Get the environment for subprocesses

        Creates a modified copy if needed based on the parameters,
        but returns the original environment otherwise.
        """
        env = None
        if self._overwrite is not None:
            env = env or self._original_env.copy()
            if self._overwrite:
                env["GRASS_OVERWRITE"] = "1"
            else:
                env["GRASS_OVERWRITE"] = "0"

        if (
            self._verbose is not None
            or self._quiet is not None
            or self._superquiet is not None
        ):
            env = env or self._original_env.copy()

            def set_or_unset(env, variable_value, state):
                """
                Set the variable the corresponding value if state is True. If it is
                False and the variable is set to the corresponding value, unset it.
                """
                if state:
                    env["GRASS_VERBOSE"] = variable_value
                elif (
                    state is False
                    and "GRASS_VERBOSE" in env
                    and env["GRASS_VERBOSE"] == variable_value
                ):
                    del env["GRASS_VERBOSE"]

            # This does not check for multiple ones set at the same time,
            # but the most verbose one wins for safety.
            set_or_unset(env, "0", self._superquiet)
            set_or_unset(env, "1", self._quiet)
            set_or_unset(env, "3", self._verbose)

        return env or self._original_env

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

        # Compute the environment for subprocesses and store it for later use.
        if "env" not in popen_options:
            popen_options["env"] = self._modified_env_if_needed()

        object_parameter_handler.translate_objects_to_data(
            kwargs, env=popen_options["env"]
        )

        # We approximate original kwargs with the possibly-modified kwargs.
        result = self._run_cmd(
            args,
            tool_kwargs=kwargs,  # We send the original kwargs for error reporting.
            input=object_parameter_handler.stdin,
            parameter_converter=object_parameter_handler,
            **popen_options,
        )
        use_objects = object_parameter_handler.translate_data_to_objects(
            kwargs, env=popen_options["env"]
        )
        if use_objects:
            if self._consistent_return_value:
                result.set_arrays(object_parameter_handler.all_array_results)
            else:
                result = object_parameter_handler.result

        if object_parameter_handler.temporary_rasters:
            self.call(
                "g.remove",
                type="raster",
                name=object_parameter_handler.temporary_rasters,
                flags="f",
            )
        return result

    def run_cmd(
        self,
        command: list[str],
        *,
        input: str | bytes | None = None,
        parameter_converter: ParameterConverter | None = None,
        tool_kwargs: dict | None = None,
        **popen_options,
    ):
        """Run a tool by passing its name and parameters a list of strings.

        The function will perform additional processing on the parameters
        such as importing GRASS native raster files to in-project data.

        :param command: list of strings to execute as the command
        :param input: text input for the standard input of the tool
        :param **popen_options: additional options for :py:func:`subprocess.Popen`
        """
        return self._run_cmd(command, input=input, **popen_options)

    def _run_cmd(
        self,
        command: list[str],
        *,
        input: str | bytes | None = None,
        parameter_converter: ParameterConverter | None = None,
        tool_kwargs: dict | None = None,
        **popen_options,
    ):
        """Run a tool by passing its name and parameters a list of strings.

        If parameters were already processed using a *ParameterConverter* instance,
        the instance can be passed as the *parameter_converter* parameter, avoiding
        re-processing.

        :param command: list of strings to execute as the command
        :param input: text input for the standard input of the tool
        :param parameter_converter: a Parameter converter instance if already used
        :param tool_kwargs: named tool arguments used for error reporting (experimental)
        :param **popen_options: additional options for :py:func:`subprocess.Popen`
        """
        # Compute the environment for subprocesses and store it for later use.
        if "env" not in popen_options:
            popen_options["env"] = self._modified_env_if_needed()

        if parameter_converter is None:
            # Parameters were not processed yet, so process them now.
            parameter_converter = ParameterConverter()
            parameter_converter.process_parameter_list(command[1:])
        try:
            # Processing parameters for import and export is costly, so we do it
            # only when we previously determined there might be such parameters.
            if parameter_converter.import_export:
                if self._importer_exporter is None:
                    # The importer exporter instance may be reused in later calls
                    # based on how the cache is used.
                    self._importer_exporter = ImporterExporter(
                        run_function=self.call, run_cmd_function=self.call_cmd
                    )
                command = self._importer_exporter.process_parameter_list(
                    command, **popen_options
                )
                # The command now has external files replaced with in-project data,
                # so now we import the data.
                self._importer_exporter.import_data(env=popen_options["env"])
            result = self.call_cmd(
                command,
                tool_kwargs=tool_kwargs,  # used in error reporting
                input=input,
                **popen_options,
            )
            if parameter_converter.import_export:
                # Exporting data inherits the overwrite flag from the command
                # if provided, otherwise it is driven by the environment.
                overwrite = None
                if "--o" in command or "--overwrite" in command:
                    overwrite = True
                self._importer_exporter.export_data(
                    env=popen_options["env"], overwrite=overwrite
                )
        finally:
            if parameter_converter.import_export:
                if not self._delete_on_context_exit and not self._use_cache:
                    # Delete the in-project data after each call.
                    self._importer_exporter.cleanup(env=popen_options["env"])
        return result

    def call(self, tool_name_: str, /, **kwargs):
        """Run a tool by specifying its name as a string and parameters.

        The parameters tool are tool name as a string and parameters as keyword
        arguments. The keyword arguments may include an argument *flags* which is a
        string of one-character tool flags.

        The function will directly execute the tool without any major processing of
        the parameters, but numbers, lists, and tuples will still be translated to
        strings for execution.

        :param tool_name_: name of a GRASS tool
        :param **kwargs: tool parameters
        """
        args, popen_options = gs.popen_args_command(tool_name_, **kwargs)
        return self.call_cmd(args, **popen_options)

    def call_cmd(self, command, tool_kwargs=None, input=None, **popen_options):
        """Run a tool by passing its name and parameters as a list of strings.

        The function is similar to :py:func:`subprocess.run` but with different
        defaults and return value.

        :param command: list of strings to execute as the command
        :param tool_kwargs: named tool arguments used for error reporting
        :param input: text input for the standard input of the tool
        :param **popen_options: additional options for :py:func:`subprocess.Popen`
        """
        # We allow the user to overwrite env, which allows for maximum flexibility
        # with some potential for confusion when the user uses a broken environment.
        if "env" not in popen_options:
            popen_options["env"] = self._modified_env_if_needed()
        if self._capture_output:
            if "stdout" not in popen_options:
                popen_options["stdout"] = gs.PIPE
        if self._capture_stderr:
            if "stderr" not in popen_options:
                popen_options["stderr"] = gs.PIPE
        if input is not None:
            popen_options["stdin"] = gs.PIPE
        else:
            popen_options["stdin"] = None
        process = gs.Popen(
            command,
            **popen_options,
        )
        stdout, stderr = process.communicate(input=input)
        returncode = process.poll()
        # We don't have the keyword arguments to pass to the resulting object.
        result = ToolResult(
            name=command[0],
            command=command,
            kwargs=tool_kwargs,
            returncode=returncode,
            stdout=stdout,
            stderr=stderr,
        )
        if returncode != 0:
            # This is only for the error states.
            # The handle_errors function handles also the run_command functions
            # and may use some overall review to make the handling of the tool name
            # and parameters more clear, but currently, the first item in args is a
            # list if it is a whole command.
            args = [command[0]] if tool_kwargs else [command]
            return gs.handle_errors(
                returncode,
                result=result,
                args=args,
                kwargs=tool_kwargs or {},
                stderr=stderr,
                handler=self._errors,
                exception=ToolError,
                env=popen_options["env"],
            )
        if not self._consistent_return_value and not result.stdout:
            return None
        return result

    def __getattr__(self, name):
        """Get a function representing a GRASS tool.

        Attribute should be in the form 'r_example_name'. For example, 'r.slope.aspect'
        is used trough attribute 'r_slope_aspect'.
        """
        if not self._name_resolver:
            self._name_resolver = ToolFunctionResolver(
                run_function=self.run,
                env=self._original_env,
            )
        return self._name_resolver.get_function(name, exception_type=AttributeError)

    def __dir__(self):
        """List available tools and standard attributes."""
        if not self._name_resolver:
            self._name_resolver = ToolFunctionResolver(
                run_function=self.run,
                env=self._original_env,
            )
        # Collect instance and class attributes
        static_attrs = set(dir(type(self))) | set(self.__dict__.keys())
        return list(static_attrs) + self._name_resolver.names()

    def __enter__(self):
        """Enter the context manager context.

        :returns: reference to the object (self)
        """
        self._delete_on_context_exit = True
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        """Exit the context manager context."""
        if not self._use_cache:
            self.cleanup()

    def cleanup(self):
        if self._importer_exporter is not None:
            self._importer_exporter.cleanup(env=self._modified_env_if_needed())
