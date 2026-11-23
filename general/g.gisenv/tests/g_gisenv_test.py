"""Tests of g.gisenv"""

import pathlib

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


def test_set_stores_variable_in_gisrc_file(tools, session):
    """set= (default store=gisrc) writes the variable into the session's gisrc file"""
    tools.g_gisenv(set="FOO=bar")

    gisrc_file = pathlib.Path(session.env["GISRC"])

    assert gisrc_file.exists()
    assert "FOO: bar" in gisrc_file.read_text(encoding="utf-8")


def test_set_store_mapset_stores_variable_in_var_file(tools):
    """store=mapset writes the variable into the mapset's VAR file on disk"""
    gisdbase = tools.g_gisenv(get="GISDBASE").text
    location = tools.g_gisenv(get="LOCATION_NAME").text
    mapset = tools.g_gisenv(get="MAPSET").text

    tools.g_gisenv(set="TESTVAR=hello", store="mapset")

    var_file = pathlib.Path(gisdbase, location, mapset, "VAR")

    assert var_file.exists()
    assert "TESTVAR: hello" in var_file.read_text(encoding="utf-8")


@pytest.fixture
def nprocs_tools(tools):
    """Tools in a session with NPROCS set to 3 and a raster map named ones"""
    tools.g_gisenv(set="NPROCS=3")
    tools.g_region(rows=2, cols=2)
    tools.r_mapcalc(expression="ones = 1")
    return tools


def test_nprocs_can_be_read_back(tools):
    """NPROCS set with g.gisenv can be read back"""
    tools.g_gisenv(set="NPROCS=3")
    assert tools.g_gisenv(get="NPROCS").text == "3"


def test_nprocs_is_default_of_tools(nprocs_tools):
    """Tools take NPROCS as the default of their nprocs option

    r.series records its options, including nprocs, in the history of its
    output.
    """
    nprocs_tools.r_series(input="ones", output="sum", method="sum")
    assert "nprocs=3" in nprocs_tools.r_info(map="sum", flags="h").text


def test_unset_nprocs_gives_option_default(nprocs_tools):
    """Without NPROCS, the nprocs option of tools has its default of 0"""
    nprocs_tools.g_gisenv(unset="NPROCS")
    nprocs_tools.r_series(input="ones", output="sum", method="sum")
    assert "nprocs=0" in nprocs_tools.r_info(map="sum", flags="h").text
