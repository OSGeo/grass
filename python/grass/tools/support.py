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
from collections import namedtuple

try:
    import numpy as np
except ImportError:
    np = None

import grass.script as gs

try:
    import grass.script.array as ga
except ImportError:
    # While np and ga are separate here, later, we will assume that if np is present,
    # ga is present as well because that's the only import-time failure we expect.
    ga = None

from .importexport import ImporterExporter


class ParameterConverter:
    """Converts parameter values to strings and facilitates flow of the data."""

    def __init__(self):
        self._numpy_inputs = {}
        self._numpy_outputs = []
        self._numpy_inputs_ordered = []
        self.stdin = None
        self.result = None
        self.temporary_rasters = []
        self.import_export = None

    def process_parameters(self, kwargs):
        """Converts high level parameter values to strings.

        Converts io.StringIO to dash and stores the string in the *stdin* attribute.
        Replaces NumPy arrays by temporary raster names and stores the arrays.
        Replaces NumPy array types by temporary raster names.

        Temporary names are accessible in the *temporary_rasters* attribute and need
        to be cleaned.
        The functions *translate_objects_to_data* and *translate_data_to_objects*
        need to be called before and after the computation to do the translations
        from NumPy arrays to GRASS data and from GRASS data to NumPy arrays.

        Simple type conversions from numbers and iterables to strings are expected to
        be done by lower level code.
        """
        for key, value in kwargs.items():
            if np and isinstance(value, np.ndarray):
                name = gs.append_uuid("tmp_serialized_input_array")
                kwargs[key] = name
                self._numpy_inputs[key] = (name, value)
            elif np and value in (np.ndarray, np.array, ga.array):
                # We test for class or the function.
                name = gs.append_uuid("tmp_serialized_output_array")
                kwargs[key] = name
                self._numpy_outputs.append((name, key, value))
            elif isinstance(value, StringIO):
                kwargs[key] = "-"
                self.stdin = value.getvalue()
            elif self.import_export is None and ImporterExporter.is_recognized_file(
                value
            ):
                self.import_export = True
        if self.import_export is None:
            self.import_export = False

    def process_parameter_list(self, command):
        """Converts or at least processes parameters passed as list of strings"""
        for item in command:
            split_ = item.split("=", maxsplit=1)
            value = split_[1] if len(split_) > 1 else item
            if self.import_export is None and ImporterExporter.is_recognized_file(
                value
            ):
                self.import_export = True
        if self.import_export is None:
            self.import_export = False

    def translate_objects_to_data(self, kwargs, env):
        """Convert NumPy arrays to GRASS data"""
        for name, value in self._numpy_inputs.values():
            map2d = ga.array(env=env)
            map2d[:] = value
            map2d.write(name)
            self.temporary_rasters.append(name)

    def translate_data_to_objects(self, kwargs, env):
        """Convert GRASS data to NumPy arrays

        Returns True if there is one or more output arrays, False otherwise.
        The arrays are stored in the *result* attribute.
        """
        output_arrays = []
        output_arrays_dict = {}
        for name, key, unused in self._numpy_outputs:
            output_array = ga.array(name, env=env)
            output_arrays.append(output_array)
            output_arrays_dict[key] = output_array
            self.temporary_rasters.append(name)
        # We create the namedtuple dynamically, so we don't use the typed version.
        self.all_array_results = namedtuple("arrays", output_arrays_dict.keys())(  # noqa: PYI024
            *output_arrays_dict.values()
        )
        if len(output_arrays) == 1:
            self.result = output_arrays[0]
            return True
        if len(output_arrays) > 1:
            self.result = tuple(output_arrays)
            return True
        self.result = None
        return False


class ToolFunctionResolver:
    def __init__(self, *, run_function, env, allowed_prefix=None):
        self._run_function = run_function
        self._env = env
        self._names = None
        self._allowed_prefix = allowed_prefix

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
        if self._allowed_prefix and not name.startswith(self._allowed_prefix):
            msg = (
                f"Tool {name} ({tool_name}) is not suitable to run this way"
                f" based on the allowed prefix ({self._allowed_prefix})"
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
        if self._allowed_prefix:
            dotted_allow_prefix = self._allowed_prefix.replace("_", ".")
        else:
            dotted_allow_prefix = None
        self._names = [
            name.replace(".", "_")
            for name in gs.get_commands()[0]
            if not dotted_allow_prefix or name.startswith(dotted_allow_prefix)
        ]
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
        self._arrays = {}

    @property
    def text(self) -> str | None:
        """Text output as decoded string without leading and trailing whitespace"""
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
            # This provides consistent behavior with explicit separator including
            # a single space and no separator which triggers general whitespace
            # splitting which results in an empty list for an empty string.
            return []
        # The use of strip is assuming that the output is one line which
        # ends with a newline character which is for display only.
        return self.text.split(separator)

    @property
    def stdout(self) -> str | bytes | None:
        """Standard output (text output) without modifications"""
        return self._stdout

    @property
    def stderr(self) -> str | bytes | None:
        """Standard error output (messages and errors) without modifications"""
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

    def __iter__(self):
        return iter(self._json_or_error())

    def __repr__(self):
        parameters = []
        parameters.append(f"returncode={self.returncode}")
        if self._stdout is not None:
            parameters.append(f"stdout='{self._stdout}'")
        if self._stderr is not None:
            parameters.append(f"stderr='{self._stderr}'")
        return f"{self.__class__.__name__}({', '.join(parameters)})"

    @property
    def arrays(self) -> dict:
        return self._arrays

    def set_arrays(self, arrays):
        self._arrays = arrays
