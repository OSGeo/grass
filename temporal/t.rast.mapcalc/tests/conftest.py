"""Fixtures for t.rast.mapcalc tests.

Each fixture creates its own temporary GRASS project and session, following
the pattern from temporal/t.rast.list/tests/conftest.py. Tests do NOT rely
on a pre-existing GRASS session.

(C) 2025 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.
"""

import os
from types import SimpleNamespace

import pytest

import grass.script as gs


def _setup_maps(session):
    """Create region, prec_* rasters, and STRDS precip_abs1, precip_abs2.

    All commands use session.env to ensure they run in the correct session.
    """
    gs.run_command("g.gisenv", set="TGIS_USE_CURRENT_MAPSET=1", env=session.env)
    gs.run_command(
        "g.region",
        s=0,
        n=80,
        w=0,
        e=120,
        b=0,
        t=50,
        res=10,
        res3=10,
        env=session.env,
    )
    # Create raster maps with seeded random values
    for name, val in [
        ("prec_1", 550),
        ("prec_2", 450),
        ("prec_3", 320),
        ("prec_4", 510),
        ("prec_5", 300),
        ("prec_6", 650),
    ]:
        gs.mapcalc(f"{name} = rand(0, {val})", seed=1, env=session.env)

    # Create STRDS datasets
    gs.run_command(
        "t.create",
        type="strds",
        temporaltype="absolute",
        output="precip_abs1",
        title="A test",
        description="A test",
        overwrite=True,
        env=session.env,
    )
    gs.run_command(
        "t.create",
        type="strds",
        temporaltype="absolute",
        output="precip_abs2",
        title="A test",
        description="A test",
        overwrite=True,
        env=session.env,
    )
    # Register maps with temporal information
    gs.run_command(
        "t.register",
        flags="i",
        type="raster",
        input="precip_abs1",
        maps="prec_1,prec_2,prec_3,prec_4,prec_5,prec_6",
        start="2001-01-01",
        increment="3 months",
        overwrite=True,
        env=session.env,
    )
    gs.run_command(
        "t.register",
        type="raster",
        input="precip_abs2",
        maps="prec_1,prec_2,prec_3,prec_4,prec_5,prec_6",
        overwrite=True,
        env=session.env,
    )


@pytest.fixture(scope="module")
def mapcalc_session_basic(tmp_path_factory):
    """Create a GRASS session with STRDS data for basic tests.

    Creates a temporary GRASS project and initializes a session. The session
    and all data are cleaned up automatically when the context exits.
    Follows the pattern from temporal/t.rast.list/tests/conftest.py.
    """
    tmp_path = tmp_path_factory.mktemp("t_rast_mapcalc_basic")
    project = tmp_path / "test"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        _setup_maps(session)
        # Yield SimpleNamespace with env for backward compatibility with tests
        yield SimpleNamespace(session=session, env=session.env)


@pytest.fixture(scope="module")
def mapcalc_session_operators(tmp_path_factory):
    """Create a GRASS session with STRDS data for operator tests.

    Identical structure to mapcalc_session_basic but creates a separate
    project to isolate operator tests from basic tests.
    """
    tmp_path = tmp_path_factory.mktemp("t_rast_mapcalc_operators")
    project = tmp_path / "test"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        _setup_maps(session)
        yield SimpleNamespace(session=session, env=session.env)
