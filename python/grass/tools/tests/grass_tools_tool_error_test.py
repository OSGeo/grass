import subprocess
import re

import pytest

from grass.exceptions import CalledModuleError
from grass.tools import ToolError


def test_exception_type_for_tools(plain_tools):
    """Check that the expected type is produced"""
    with pytest.raises(ToolError):
        plain_tools.g_region(raster="does_not_exist")


def test_compatibility_with_subprocess(plain_tools):
    """We want tools to work as a smooth replacement of subprocess calls"""
    with pytest.raises(subprocess.CalledProcessError):
        plain_tools.g_region(raster="does_not_exist")


def test_compatibility_with_run_command(plain_tools):
    """This is for backwards compatibility and may be removed in the future"""
    with pytest.raises(CalledModuleError):
        plain_tools.g_region(raster="does_not_exist")


def test_tool_name_in_message(plain_tools):
    """Check tool name appears in the message"""
    with pytest.raises(ToolError, match=r"g\.region"):
        plain_tools.g_region(raster="does_not_exist")


def test_parameter_in_message(plain_tools):
    """Check parameters apprear in the message"""
    with pytest.raises(ToolError, match=r"raster.*does_not_exist"):
        plain_tools.g_region(raster="does_not_exist")


def test_flag_in_message(plain_tools):
    """Check flags parameter value appears in the message"""
    with pytest.raises(ToolError, match=r"[^a-zA-Z]*p[^a-zA-Z]"):
        plain_tools.g_region(raster="does_not_exist", flags="p")


def test_one_line_message_formatting(plain_tools):
    """Check how one line message is formatted"""
    with pytest.raises(ToolError, match=r"raster.*does_not_exist") as exc_info:
        plain_tools.g_region(raster="does_not_exist")

    lines = str(exc_info.value).splitlines()

    assert "does_not_exist" in lines[0]  # raster name
    assert "Raster" in lines[0]  # piece of the message
    assert "not found" in lines[0]  # piece of the message

    assert "ERROR" not in lines  # we are trying to remove it

    assert "does_not_exist" in lines[1]  # parameter value
    # We show what was actually executed as a subprocess:
    assert "g.region" in lines[1]  # actual tool name
    assert "raster=" in lines[1]  # parameter name
    assert len(lines) == 2  # we expect only 2 lines here
    # Ideally, this would be a list, but with passing things thru handle_errors
    # we end up with a string (which is good for making the message).
    assert exc_info.value.cmd == "g.region raster=does_not_exist"


def test_mutli_line_message_formatting(plain_tools):
    """Check how multi line message is formatted"""
    with pytest.raises(ToolError, match=r"raster.*does_not_exist") as exc_info:
        plain_tools.g_region(
            raster="does_not_exist",
            does_not_exist="abc",
            does_not_exist_2=2,
            does_not_exist_3=3,
        )

    lines = str(exc_info.value).splitlines()

    assert "Run" in lines[0]  # raster name
    assert "g.region" in lines[0]  # actual tool name

    remaining_lines = "\n".join(lines[1:])
    assert "does_not_exist" in remaining_lines
    assert "does_not_exist_2" in remaining_lines
    assert "does_not_exist_3" in remaining_lines


def test_attributes_and_str():
    """Check error object attributes"""
    returncode = 42
    errors = "error output text"
    command = ["r.test.tool", "first=value", "second=value"]

    value = ToolError("r.test.tool", command, returncode, errors)

    # Attributes
    assert value.returncode == returncode
    assert value.errors == errors

    # Resulting string
    assert re.search(r"r\.test\.tool.*first=value.*second=value", str(value))
    assert "error output text" in str(value)

    # Undocumented members
    assert value.tool == command[0]

    # grass.exceptions.CalledModuleError compatibility
    assert value.code == command
    assert value.module == command[0]
    assert isinstance(value.msg, str)
    assert len(value.msg) > 0

    # subprocess compatibility
    assert value.stderr == errors
    assert value.cmd == command
    # Not implemented yet subprocess compatibility parts, so always None.
    assert value.output is None
    assert value.stdout is None
