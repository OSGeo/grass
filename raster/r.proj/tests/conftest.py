"""Fixtures for the r.proj parallel-correctness pytest.

Builds one GISDBASE holding an EPSG:4326 source project with two small
generated input rasters, plus EPSG:3857 and EPSG:3413 (north polar
stereographic) destination projects. r.proj reprojects from the source
into the active destination session; the tests compare the module's own
serial and parallel runs.

The input is integer CELL with values well below 2^24
(row()*100 + col() + (row()*row()+col()*col())%13, max ~5058), so it survives
a float32 round-trip losslessly. This is deliberate: the forced tile-cache
path reads through the FCELL readcell cache while the banded nearest path
reads the native type, so only a float32-exact input keeps the forced-fallback
bitwise assert valid (a DCELL input would diverge by float32 quantization
alone). The (row()*row()+col()*col())%13 term gives the surface enough
curvature that bilinear and bicubic interpolation diverge past the reference
test's rel=1e-7 tolerance (a linear ramp, or a milder term, leaves their
statistics identical or within tolerance), which the method reference test
relies on to catch an _f-kernel dispatch swap. Values depend on
grid position only (no trig, no random), so they are bit-identical across
platforms and resolutions. Both rasters are 50x50 to stay well under the CI
time budget.
"""

# Duplicated from the companion test-split PR (branch fix-rproj-tests),
# which owns this file alongside the method reference tests. Drop this copy
# when that PR merges into main; the fixtures are identical.

import os

import pytest

import grass.script as gs

INPUT_EXPRESSION = "row() * 100 + col() + (row() * row() + col() * col()) % 13"

SRC_PROJECT = "src4326"
# Mid-latitude box for the 3857 identity/fallback cases.
INPUT_MID = "input_mid"
# High-latitude, full-longitude box so a north-polar frame has data to read.
INPUT_POLAR = "input_polar"


@pytest.fixture(scope="module")
def gisdbase_with_source(tmp_path_factory):
    """GISDBASE containing src4326 with the mid and polar input rasters."""
    gisdbase = tmp_path_factory.mktemp("rproj_parallel")
    gs.create_project(gisdbase / SRC_PROJECT, epsg="4326")
    with gs.setup.init(gisdbase / SRC_PROJECT, env=os.environ.copy()) as session:
        env = session.env
        gs.run_command("g.region", n=41, s=40, w=-100, e=-99, rows=50, cols=50, env=env)
        gs.run_command(
            "r.mapcalc", expression=f"{INPUT_MID} = {INPUT_EXPRESSION}", env=env
        )
        gs.run_command("g.region", n=89, s=50, w=-180, e=180, rows=50, cols=50, env=env)
        gs.run_command(
            "r.mapcalc", expression=f"{INPUT_POLAR} = {INPUT_EXPRESSION}", env=env
        )
    return gisdbase


@pytest.fixture(scope="module")
def session_3857(gisdbase_with_source):
    """Active session in an EPSG:3857 destination project."""
    gs.create_project(gisdbase_with_source / "dst3857", epsg="3857")
    with gs.setup.init(
        gisdbase_with_source / "dst3857", env=os.environ.copy()
    ) as session:
        yield session


@pytest.fixture(scope="module")
def session_pole(gisdbase_with_source):
    """Active session in an EPSG:3413 (north polar stereographic) project."""
    gs.create_project(gisdbase_with_source / "dst_pole", epsg="3413")
    with gs.setup.init(
        gisdbase_with_source / "dst_pole", env=os.environ.copy()
    ) as session:
        yield session
