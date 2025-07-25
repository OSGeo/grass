"""Tests of grass.tools.support.ToolFunctionResolver"""

import pytest

from grass.tools.support import ToolFunctionResolver


class CustomException(Exception):
    pass


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
