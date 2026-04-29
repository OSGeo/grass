"""Fixtures for t.rast.mapcalc tests.

(C) 2025 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.
"""

import os

import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.fixture(scope="session")
def mapcalc_session(tmp_path_factory):
    """GRASS session with prec_1..prec_6 rasters and precip_abs1/2 STRDS.

    Session-scoped: setup (region, rasters, STRDS, TGIS init) runs once for
    the whole test run. Tests use overwrite=True for their outputs and
    test_failure_on_missing_map restores its rename in a finally block, so
    sharing across modules is safe.
    """
    tmp_path = tmp_path_factory.mktemp("t_rast_mapcalc")
    project = tmp_path / "test"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session, overwrite=True)
        tools.g_gisenv(set="TGIS_USE_CURRENT_MAPSET=1")
        tools.g_region(s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10)
        for name, val in [
            ("prec_1", 550),
            ("prec_2", 450),
            ("prec_3", 320),
            ("prec_4", 510),
            ("prec_5", 300),
            ("prec_6", 650),
        ]:
            tools.r_mapcalc(expression=f"{name} = rand(0, {val})", seed=1)
        for name in ("precip_abs1", "precip_abs2"):
            tools.t_create(
                type="strds",
                temporaltype="absolute",
                output=name,
                title="A test",
                description="A test",
            )
        tools.t_register(
            flags="i",
            type="raster",
            input="precip_abs1",
            maps="prec_1,prec_2,prec_3,prec_4,prec_5,prec_6",
            start="2001-01-01",
            increment="3 months",
        )
        tools.t_register(
            type="raster",
            input="precip_abs2",
            maps="prec_1,prec_2,prec_3,prec_4,prec_5,prec_6",
        )
        yield session


@pytest.fixture
def assert_tinfo():
    """Return a helper that checks t.info -g output against expected key/value pairs."""

    def _assert(tools, strds_name, expected):
        actual = tools.t_info(type="strds", input=strds_name, flags="g").keyval
        for key, value in expected.items():
            assert key in actual, f"missing key {key!r} in t.info output"
            assert actual[key] == value, (
                f"{key}: expected {value!r}, got {actual[key]!r}"
            )

    return _assert
