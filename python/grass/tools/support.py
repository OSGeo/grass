##############################################################################
# AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>
#
# PURPOSE:   API to call GRASS tools (modules) as Python functions
#
# COPYRIGHT: (C) 2025 Vaclav Petras and the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.
##############################################################################

"""
The module provides support functionality for calling GRASS tools (modules)
as Python functions.

Do not use this module directly unless you are developing GRASS or its wrappers.
In that case, be prepared to update your code if this module changes. This module may
change between releases.
"""

from __future__ import annotations

import json
import shutil
from io import StringIO

import grass.script as gs


class ParameterConverter:
    def __init__(self):
        self._numpy_inputs = {}
        self._numpy_outputs = {}
        self._numpy_inputs_ordered = []
        self.stdin = None

    def process_parameters(self, kwargs):
        for key, value in kwargs.items():
            if isinstance(value, StringIO):
                kwargs[key] = "-"
                self.stdin = value.getvalue()


class ToolFunctionResolver:
    def __init__(self, *, run_function, env):
        self._run_function = run_function
        self._env = env
        self._names = None

    def get_tool_name(self, name, exception_type):
        """Parse attribute to GRASS display module. Attribute should be in
        the form 'tool_name'. For example, 'r.info' is called with 'r_info'.
        """
        # Convert snake case attribute name to dotted tool name.
        tool_name = name.replace("_", ".")
        # We first try to find the tool on path which is much faster than getting
        # and checking the names, but if the tool is not found, likely because runtime
        # is not set up, we check the names.
        if (
            not shutil.which(tool_name, path=self._env["PATH"])
            and name not in self.names()
        ):
            suggestions = self.suggest_tools(tool_name)
            if suggestions:
                # While Python may automatically suggest the closest match,
                # we show more matches. We also show single match more often
                # (this may change in the future).
                msg = (
                    f"Tool {name} ({tool_name}) not found"
                    f" (but found {', '.join(suggestions)})"
                )
                raise exception_type(msg)
            msg = (
                f"Tool or attribute {name} ({tool_name}) not found"
                " (check session setup and documentation for tool and attribute names)"
            )
            raise exception_type(msg)
        return tool_name

    def get_function(self, name, exception_type):
        tool_name = self.get_tool_name(name, exception_type)

        def wrapper(**kwargs):
            return self._run_function(tool_name, **kwargs)

        wrapper.__doc__ = f"Run {tool_name} as function"

        return wrapper

    def __getattr__(self, name):
        return self.get_function(name, exception_type=AttributeError)

    @staticmethod
    def levenshtein_distance(text1: str, text2: str) -> int:
        if len(text1) < len(text2):
            return ToolFunctionResolver.levenshtein_distance(text2, text1)

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

    def suggest_tools(self, text):
        """Suggest matching tool names based on provided text.

        The first five tools matching the condition are returned,
        so this always returns at most five names, but it may not be the best matches.
        The returned names are sorted alphabetically (not by priority).
        This specific behavior may change in the future versions.
        """
        result = []
        max_suggestions = 5
        for name in self.names():
            if ToolFunctionResolver.levenshtein_distance(text, name) < len(text) / 2:
                result.append(name)
            if len(result) >= max_suggestions:
                break
        result.sort()
        return result

    def names(self):
        if self._names:
            return self._names
        self._names = [name.replace(".", "_") for name in gs.get_commands()[0]]
        return self._names


class ToolResult:
    """Result returned after executing a tool"""

    def __init__(self, *, name, command, kwargs, returncode, stdout, stderr):
        self._name = name
        self._command = command
        self._kwargs = kwargs
        self.returncode = returncode
        self._stdout = stdout
        self._stderr = stderr
        self._text = None
        self._cached_json = None

    @property
    def text(self) -> str | None:
        """Text output as decoded string"""
        if self._text is not None:
            return self._text
        if self._stdout is None:
            return None
        if isinstance(self._stdout, bytes):
            decoded_stdout = gs.decode(self._stdout)
        else:
            decoded_stdout = self._stdout
        self._text = decoded_stdout.strip()
        return self._text

    @property
    def keyval(self) -> dict:
        """Text output read as key-value pairs separated by equal signs

        If possible, values are converted to int or float. Empty values result in
        an empty string value (there is no recognized null value).
        Empty or no output results in an empty dictionary.
        """

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
    def comma_items(self) -> list:
        """Text output read as comma-separated list

        Empty or no output results in an empty list.
        """
        return self.text_split(",")

    @property
    def space_items(self) -> list:
        """Text output read as whitespace-separated list

        Empty or no output results in an empty list.
        """
        return self.text_split(separator=None)

    def text_split(self, separator=None) -> list:
        """Parse text output read as list separated by separators

        Any leading or trailing newlines are removed prior to parsing.
        Empty or no output results in an empty list.
        """
        if not self.text:
            # This provides consitent behavior with explicit separator including
            # a single space and no separator which triggers general whitespace
            # splitting which results in an empty list for an empty string.
            return []
        # The use of strip is assuming that the output is one line which
        # ends with a newline character which is for display only.
        return self.text.split(separator)

    @property
    def stdout(self) -> str | bytes | None:
        return self._stdout

    @property
    def stderr(self) -> str | bytes | None:
        return self._stderr

    def _json_or_error(self) -> dict:
        if self._cached_json is not None:
            return self._cached_json
        if self._stdout:
            # We are testing just std out and letting rest to the parse and the user.
            # This makes no assumption about how JSON is produced by the tool.
            try:
                self._cached_json = json.loads(self._stdout)
                return self._cached_json
            except json.JSONDecodeError as error:
                if self._kwargs and self._kwargs.get("format") == "json":
                    raise
                if self._command and "format=json" in self._command:
                    raise
                msg = (
                    f"Output of {self._name} cannot be parsed as JSON. "
                    'Did you use format="json"?'
                )
                # We don't raise JSONDecodeError ourselves, but use more general
                # ValueError when format is likely not set properly or when there is
                # no output (see below).
                # JSONDecodeError is ValueError, so users may just catch ValueError.
                raise ValueError(msg) from error
        msg = f"No text output for {self._name} to be parsed as JSON"
        raise ValueError(msg)

    @property
    def json(self) -> dict:
        """Text output read as JSON

        This returns the nested structure of dictionaries and lists or fails when
        the output is not JSON.
        """
        return self._json_or_error()

    def __getitem__(self, name):
        return self._json_or_error()[name]

    def __len__(self):
        return len(self._json_or_error())

    def __repr__(self):
        parameters = []
        parameters.append(f"returncode={self.returncode}")
        if self._stdout is not None:
            parameters.append(f"stdout='{self._stdout}'")
        if self._stderr is not None:
            parameters.append(f"stderr='{self._stderr}'")
        return f"{self.__class__.__name__}({', '.join(parameters)})"
