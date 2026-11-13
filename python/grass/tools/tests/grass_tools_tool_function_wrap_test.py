"""Tests of grass.tools.support.ToolFunctionResolver"""

import os
import sys

import pytest

from grass.tools.support import ToolFunctionResolver


class CustomException(Exception):
    pass


def create_executable(directory, name):
    """Create an empty file which is found on path as an executable"""
    if sys.platform == "win32":
        # Without an extension from PATHEXT, the file is not found.
        name = f"{name}.bat"
    path = directory / name
    path.touch()
    path.chmod(0o755)


def test_get_tool_name(echoing_resolver):
    """Check that dotted tool name is resolved"""
    assert (
        echoing_resolver.get_tool_name("r_info", exception_type=CustomException)
        == "r.info"
    )


def test_get_tool_name_exception(echoing_resolver):
    """Check that custom exception is raised when tool does not exist"""
    with pytest.raises(CustomException, match="does_not_exist"):
        assert echoing_resolver.get_tool_name(
            "does_not_exist", exception_type=CustomException
        )


def test_get_function(echoing_resolver):
    """Check that function is called (this makes use of echoing resolver)"""
    assert (
        echoing_resolver.get_function("r_info", exception_type=CustomException)()
        == "r.info"
    )


def test_get_function_exception(echoing_resolver):
    """Check that custom exception is raised when tool does not exist"""
    with pytest.raises(CustomException, match="does_not_exist"):
        assert echoing_resolver.get_function(
            "does_not_exist", exception_type=CustomException
        )


def test_attribute(echoing_resolver):
    """Check that function is called (this makes use of echoing resolver)"""
    assert echoing_resolver.r_info() == "r.info"


def test_attribute_exception(echoing_resolver):
    """Check that attribute error is raised with attribute access"""
    with pytest.raises(AttributeError, match="does_not_exist"):
        assert echoing_resolver.does_not_exist


def test_attribute_with_underscores_in_tool_name(tmp_path):
    """Check that underscores which are part of the tool name are kept"""
    create_executable(tmp_path, "r.example.name_with_underscores")
    resolver = ToolFunctionResolver(
        run_function=lambda x: x, env={"PATH": str(tmp_path)}
    )
    assert (
        resolver.r_example_name_with_underscores() == "r.example.name_with_underscores"
    )


def test_attribute_prefers_dots_in_tool_name(tmp_path):
    """Check that dots are used when both tool names exist"""
    create_executable(tmp_path, "r.example.name")
    create_executable(tmp_path, "r.example_name")
    resolver = ToolFunctionResolver(
        run_function=lambda x: x, env={"PATH": str(tmp_path)}
    )
    assert resolver.r_example_name() == "r.example.name"


def test_names(echoing_resolver):
    """Check that tool names are present with underscores, not dots"""
    assert "r_info" in echoing_resolver.names()
    assert "v_info" in echoing_resolver.names()
    assert "r.info" not in echoing_resolver.names()
    assert "v.info" not in echoing_resolver.names()


def test_levenshtein_distance_empty_text():
    empty_text = ""
    non_empty_text = "abc"
    ToolFunctionResolver.levenshtein_distance(empty_text, non_empty_text) == len(
        non_empty_text
    )
    ToolFunctionResolver.levenshtein_distance(non_empty_text, empty_text) == len(
        non_empty_text
    )


def test_allowed_prefix_no_separator():
    """Check that dotted tool name is resolved"""
    prefix = "r"
    resolver = ToolFunctionResolver(
        run_function=lambda x: x, env=os.environ.copy(), allowed_prefix=prefix
    )
    assert resolver.r_watershed() == "r.watershed"
    assert "r_info" in resolver.names()
    for name in resolver.names():
        assert name.startswith(prefix)
    with pytest.raises(AttributeError, match="does_not_exist"):
        assert resolver.does_not_exist
    with pytest.raises(AttributeError, match="r_does_not_exist"):
        assert resolver.r_does_not_exist
    with pytest.raises(AttributeError, match="v_does_not_exist"):
        assert resolver.v_does_not_exist
    with pytest.raises(AttributeError, match="v_info"):
        assert resolver.v_info
    with pytest.raises(TypeError, match="v_info"):
        assert resolver.get_function("v_info", exception_type=TypeError)


def test_allowed_prefix_d_with_underscore():
    """Check that dotted tool name is resolved"""
    prefix = "d_"
    resolver = ToolFunctionResolver(
        run_function=lambda x: x, env=os.environ.copy(), allowed_prefix=prefix
    )
    assert resolver.d_rast() == "d.rast"
    assert "d_vect" in resolver.names()
    for name in resolver.names():
        assert name.startswith(prefix)
    with pytest.raises(AttributeError, match="does_not_exist"):
        assert resolver.does_not_exist
    with pytest.raises(AttributeError, match="d_does_not_exist"):
        assert resolver.d_does_not_exist
    with pytest.raises(AttributeError, match="v_does_not_exist"):
        assert resolver.v_does_not_exist
    with pytest.raises(AttributeError, match="r_info"):
        assert resolver.r_info
    with pytest.raises(TypeError, match="r_info"):
        assert resolver.get_function("r_info", exception_type=TypeError)
