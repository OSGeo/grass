"""r.mapcalc tests focused on multiple processes"""

import pytest

from grass.tools import Tools


@pytest.mark.parametrize("nprocs", [0, 1, 4])
def test_create_constant(session_in_mapset, nprocs):
    tools = Tools(session=session_in_mapset)
    tools.r_mapcalc(expression="test = 10", nprocs=nprocs)
    result = tools.r_info(map="test", format="json")
    assert result["mean"] == 10
    assert result["min"] == 10
    assert result["max"] == 10
    assert result["cells"] == 30


@pytest.mark.parametrize("nprocs", [0, 1, 4])
def test_syntax_error(session_in_mapset, nprocs):
    tools = Tools(
        session=session_in_mapset, consistent_return_value=True, errors="ignore"
    )
    result = tools.r_mapcalc(expression="test = double(", nprocs=nprocs)
    assert result.returncode == 1
    assert "syntax error" in result.stderr


@pytest.mark.parametrize("nprocs", [0, 1, 4])
def test_raster_does_not_exist(session_in_mapset, nprocs):
    tools = Tools(
        session=session_in_mapset, consistent_return_value=True, errors="ignore"
    )
    result = tools.r_mapcalc(expression="test = does_not_exist", nprocs=nprocs)
    assert result.returncode == 1
    assert "does_not_exist" in result.stderr.lower()


@pytest.mark.skip(reason="Waiting for a fix to work on all platforms for any nprocs")
@pytest.mark.parametrize("nprocs", [0, 1, *list(range(2, 11, 2))])
def test_rand_no_explicit_seed_setting(session_in_mapset, nprocs):
    tools = Tools(
        session=session_in_mapset, consistent_return_value=True, errors="ignore"
    )
    result = tools.r_mapcalc(expression="test = rand(-15.0, 5.0)", nprocs=nprocs)
    assert result.returncode == 0


@pytest.mark.parametrize("nprocs", [0, 1, 4])
def test_too_many_arguments(session_in_mapset, nprocs):
    tools = Tools(
        session=session_in_mapset, consistent_return_value=True, errors="ignore"
    )
    result = tools.r_mapcalc(expression="test = double(10, 20, 30, 40)", nprocs=nprocs)
    assert result.returncode == 1
    assert "too many arguments" in result.stderr.lower()
    assert "double" in result.stderr
