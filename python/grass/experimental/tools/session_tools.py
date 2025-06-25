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

import json
import os

import grass.script as gs

from .support import ToolResult, ParameterConverter, ToolFunctionResolver


class Tools:
    """Use GRASS tools through function calls

    GRASS tools (modules) can be executed as methods of this class.
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
    ):
        """
        If session is provided and has an env attribute, it is used to execute tools.
        If env is provided, it is used to execute tools. If both session and env are
        provided, env is used to execute tools and session is ignored.
        However, session and env interaction may change in the future.

        If overwrite is provided, a global overwrite is set for all the tools.
        When overwrite is set to False, individual tool calls can set overwrite
        to True. If overwrite is set in the session or env, it is used.
        Note that once overwrite is set to True globally, an individual tool call
        cannot set it back to False.

        If verbose, quiet, superquiet is set to True, the corresponding verbosity level
        is set for all the tools. If one of them is set to False and the environment
        has the corresponding variable set, it is unset.
        The values cannot be combined. If multiple are set to True, the most verbose
        one wins.
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
        self._name_resolver = None

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

    def _process_parameters(self, command: list[str], **popen_options):
        """Get processed parameters from the tool itself"""
        popen_options["stdin"] = None
        popen_options["stdout"] = gs.PIPE
        # We respect whatever is in the stderr option because that's what the user
        # asked for and will expect to get in case of error (we pretend that it was
        # the intended run, not our special run before the actual run).
        interface_result = self.call_cmd([*command, "--json"], **popen_options)
        return json.loads(interface_result.stdout)

    def run(self, name, /, **kwargs):
        """Run a tool by specifying its name as a string and passing named arguments.

        :param name: name of the GRASS tool
        :param kwargs: named tool arguments
        """
        # Object parameters are handled first before the conversion of the call to a
        # list of strings happens.
        object_parameter_handler = ParameterConverter()
        object_parameter_handler.process_parameters(kwargs)

        # Get a fixed env parameter at at the beginning of each execution,
        # but repeat it every time in case the referenced environment is modified.
        args, popen_options = gs.popen_args_command(name, **kwargs)
        if "env" not in popen_options:
            popen_options["env"] = self._modified_env_if_needed()

        parameters = self._process_parameters(args, **popen_options)

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
        command: list[str],
        tool_kwargs=None,
        stdin=None,
        processed_parameters=None,
        **popen_options,
    ):
        if "env" not in popen_options:
            popen_options["env"] = self._modified_env_if_needed()
        if not processed_parameters:
            processed_parameters = self._process_parameters(command, **popen_options)

        # We approximate tool_kwargs as original kwargs.
        return self.call_cmd(
            command,
            tool_kwargs=tool_kwargs,
            stdin=stdin,
            **popen_options,
        )

    def call(self, name: str, /, *, tool_kwargs=None, stdin=None, **kwargs):
        args, popen_options = gs.popen_args_command(name, **kwargs)
        if "env" not in popen_options:
            popen_options["env"] = self._modified_env_if_needed()
        return self.call_cmd(
            args, tool_kwargs=tool_kwargs, stdin=stdin, **popen_options
        )

    # Make this an overload of run.
    def call_cmd(self, command, tool_kwargs=None, stdin=None, **popen_options):
        # We need to pass our own env parameter, but through that we are also allowing
        # the user to overwrite env, which allows for maximum flexibility
        # with some potential confusion when the user a broken environment.
        if "env" not in popen_options:
            popen_options["env"] = self._modified_env_if_needed()
        if self._capture_output:
            if "stdout" not in popen_options:
                popen_options["stdout"] = gs.PIPE
            if "stderr" not in popen_options:
                popen_options["stderr"] = gs.PIPE
        if stdin:
            stdin_pipe = gs.PIPE
        else:
            stdin_pipe = None
            stdin = None
        # Use text mode by default
        if "text" not in popen_options and "universal_newlines" not in popen_options:
            popen_options["text"] = True
        process = gs.Popen(
            command,
            stdin=stdin_pipe,
            **popen_options,
        )
        stdout, stderr = process.communicate(input=stdin)
        if stderr:
            stderr = gs.utils.decode(stderr)
        returncode = process.poll()
        # TODO: solve tool_kwargs is None
        # We don't have the keyword arguments to pass to the resulting object.
        result = ToolResult(
            name=command[0],
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
            )
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
