"""Tests of g.gisenv"""

import pytest

from grass.tools import ToolError


def test_get_known_variable(tools):
    """A variable that is always set in a session can be read"""
    assert tools.g_gisenv(get="MAPSET").text == "PERMANENT"


def test_get_is_case_insensitive(tools):
    """Variable names are not case sensitive"""
    assert tools.g_gisenv(get="mapset").text == "PERMANENT"


def test_get_multiple_with_separator(tools):
    """Multiple variables can be read at once, joined by sep"""
    gisdbase = tools.g_gisenv(get="GISDBASE").text
    location = tools.g_gisenv(get="LOCATION_NAME").text
    mapset = tools.g_gisenv(get="MAPSET").text

    result = tools.g_gisenv(get="GISDBASE,LOCATION_NAME,MAPSET", sep="/")

    assert result.text == f"{gisdbase}/{location}/{mapset}"


def test_get_undefined_variable_fails(tools):
    """Reading a variable that was never set is an error"""
    with pytest.raises(ToolError):
        tools.g_gisenv(get="NOSUCHVAR")


def test_set_and_get_variable(tools):
    """A variable set with set= can be read back with get="""
    tools.g_gisenv(set="FOO=bar")
    assert tools.g_gisenv(get="FOO").text == "bar"


def test_set_empty_value_unsets_variable(tools):
    """set=NAME= with an empty value removes the variable, like unset="""
    tools.g_gisenv(set="FOO=bar")
    tools.g_gisenv(set="FOO=")
    with pytest.raises(ToolError):
        tools.g_gisenv(get="FOO")


def test_unset_variable(tools):
    """unset= removes a variable"""
    tools.g_gisenv(set="FOO=bar")
    tools.g_gisenv(unset="FOO")
    with pytest.raises(ToolError):
        tools.g_gisenv(get="FOO")


def test_unset_protected_variable_is_noop(tools):
    """Unsetting a mandatory variable (e.g. MAPSET) warns but leaves it intact"""
    tools.g_gisenv(unset="MAPSET")
    assert tools.g_gisenv(get="MAPSET").text == "PERMANENT"


def test_print_all_plain(tools):
    """The -n flag prints NAME=value pairs without shell quoting"""
    result = tools.g_gisenv(flags="n")
    assert "MAPSET=PERMANENT" in result.text.splitlines()


def test_print_all_shell(tools):
    """The -s flag prints shell-quoted NAME='value'; pairs"""
    result = tools.g_gisenv(flags="s")
    assert "MAPSET='PERMANENT';" in result.text.splitlines()


def test_store_mapset_is_separate_from_gisrc(tools):
    """store=mapset writes to a different namespace than the default gisrc store"""
    tools.g_gisenv(set="TESTVAR=hello", store="mapset")

    with pytest.raises(ToolError):
        tools.g_gisenv(get="TESTVAR")

    assert tools.g_gisenv(get="TESTVAR", store="mapset").text == "hello"
