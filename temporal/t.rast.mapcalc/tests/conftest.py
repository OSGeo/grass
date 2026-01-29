"""Fixtures for t.rast.mapcalc tests.

Assumes an active GRASS session (e.g. grass --tmp-location XY --exec pytest ...).
Only prepares and cleans up raster maps and STRDS.
"""

import os
from types import SimpleNamespace

import pytest

import grass.script as gs


def setup_maps(use_seed):
    """Create region, prec_* rasters, and STRDS precip_abs1, precip_abs2."""
    gs.run_command("g.gisenv", set="TGIS_USE_CURRENT_MAPSET=1")
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
    )
    # rand() requires a seeded PRNG; use flags="s" in all cases
    for name, val in [
        ("prec_1", 550),
        ("prec_2", 450),
        ("prec_3", 320),
        ("prec_4", 510),
        ("prec_5", 300),
        ("prec_6", 650),
    ]:
        gs.run_command(
            "r.mapcalc",
            expression=f"{name} = rand(0, {val})",
            flags="s",
            overwrite=True,
        )
    gs.run_command(
        "t.create",
        type="strds",
        temporaltype="absolute",
        output="precip_abs1",
        title="A test",
        description="A test",
        overwrite=True,
    )
    gs.run_command(
        "t.create",
        type="strds",
        temporaltype="absolute",
        output="precip_abs2",
        title="A test",
        description="A test",
        overwrite=True,
    )
    gs.run_command(
        "t.register",
        flags="i",
        type="raster",
        input="precip_abs1",
        maps="prec_1,prec_2,prec_3,prec_4,prec_5,prec_6",
        start="2001-01-01",
        increment="3 months",
        overwrite=True,
    )
    gs.run_command(
        "t.register",
        type="raster",
        input="precip_abs2",
        maps="prec_1,prec_2,prec_3,prec_4,prec_5,prec_6",
        overwrite=True,
    )


def cleanup_maps():
    """Remove STRDS and raster maps created by setup_maps or tests.

    Teardown must be idempotent: STRDS or maps may already have been removed
    by tests (e.g. test_failure_on_missing_map). Use errors='ignore' so missing
    datasets/maps do not raise during teardown.
    """
    gs.run_command(
        "t.remove",
        flags="df",
        type="strds",
        inputs="precip_abs1,precip_abs2,precip_abs3,precip_abs4",
        quiet=True,
        errors="ignore",
    )
    gs.run_command(
        "g.remove",
        flags="f",
        type="raster",
        name="prec_1,prec_2,prec_3,prec_4,prec_5,prec_6",
        quiet=True,
        errors="ignore",
    )
    gs.run_command(
        "g.remove",
        flags="f",
        type="raster",
        pattern="new_prec_*",
        quiet=True,
        errors="ignore",
    )


@pytest.fixture(scope="module")
def mapcalc_session_basic():
    """STRDS data for basic tests."""
    setup_maps(use_seed=False)
    yield SimpleNamespace(env=os.environ)
    cleanup_maps()


@pytest.fixture(scope="module")
def mapcalc_session_operators():
    """STRDS data for operator tests."""
    setup_maps(use_seed=True)
    yield SimpleNamespace(env=os.environ)
    cleanup_maps()
