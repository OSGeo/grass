"""Comprehensive r.proj tests – flags and project-creation smoke tests.

The ``grass_session`` fixture from conftest.py creates a UTM project and
initialises a session.  The tests here also create an additional lat/lon
project in a second temporary directory as a smoke test.
"""

import os

import pytest
import grass.script as gs
from grass.exceptions import CalledModuleError


# ---------------------------------------------------------------------------
# fixture – second (geographic) project
# ---------------------------------------------------------------------------
@pytest.fixture(scope="module")
def latlon_project(grass_session, tmp_path_factory):  # noqa: ARG001
    """Create a geographic WGS84 project (EPSG 4326) in a temp dir.

    grass_session is requested so the main session is already active before
    we call gs.create_project.
    """
    path = str(tmp_path_factory.mktemp("grass_latlon"))
    gs.create_project(path, epsg="4326")
    return path


@pytest.fixture()
def utm_raster(temp_region):  # noqa: ARG001
    """Set a UTM region and create a test raster; region is restored after.

    Depends on temp_region (which depends on grass_session) so the session
    is active and the region is saved/restored automatically.
    """
    gs.run_command("g.region", n=4000000, s=3900000, e=650000, w=550000, res=100)
    gs.run_command(
        "r.mapcalc",
        expression="test_utm_raster = row() * 10 + col()",
        overwrite=True,
    )
    yield
    gs.run_command("g.remove", type="raster", name="test_utm_raster", flags="f")


# ---------------------------------------------------------------------------
# tests
# ---------------------------------------------------------------------------
class TestRProjCRSTransformations:
    """Bounds-flag tests using a raster in the session project."""

    def test_print_bounds_flag_p(self, utm_raster, latlon_project):  # noqa: ARG002
        """``-p`` flag must print human-readable bounds without reprojecting."""
        result = gs.read_command(
            "r.proj",
            input="test_utm_raster",
            output="_dummy_",
            flags="p",
        )
        assert len(result.strip()) > 0, "-p produced no output"
        lower = result.lower()
        assert "north" in lower or "n=" in lower or "rows" in lower

    def test_print_bounds_flag_g(self, utm_raster, latlon_project):  # noqa: ARG002
        """``-g`` flag must print machine-parseable bounds."""
        result = gs.read_command(
            "r.proj",
            input="test_utm_raster",
            output="_dummy_",
            flags="g",
        )
        for key in ("n=", "s=", "e=", "w="):
            assert key in result, f"-g output missing '{key}'"


class TestRProjLocationCreation:
    """Smoke tests confirming the test projects were created correctly."""

    def test_utm_project_has_utm_crs(self, grass_session):  # noqa: ARG001
        """The session project (UTM 26917) must report a projected CRS."""
        proj_info = gs.parse_command("g.proj", flags="g")
        proj_str = str(proj_info).lower()
        assert "utm" in proj_str or "zone" in proj_str or "26917" in proj_str

    def test_latlon_project_exists(self, latlon_project):
        """The lat/lon project directory must exist on disk."""
        assert os.path.isdir(latlon_project)


# ---------------------------------------------------------------------------
# stand-alone parametrised tests
# ---------------------------------------------------------------------------
@pytest.mark.parametrize("method", ["nearest", "bilinear", "bicubic"])
def test_interpolation_method_accepted(temp_region, method):
    """Every interpolation keyword must be accepted without error."""
    gs.run_command("g.region", n=50, s=0, e=50, w=0, res=1)
    gs.run_command(
        "r.mapcalc", expression="test_interp_comp_in = 1", overwrite=True
    )

    gs.run_command(
        "r.proj",
        input="test_interp_comp_in",
        output=f"test_interp_comp_{method}",
        method=method,
        overwrite=True,
    )

    assert gs.find_file(f"test_interp_comp_{method}", element="raster")["name"]
    gs.run_command(
        "g.remove",
        type="raster",
        name=f"test_interp_comp_in,test_interp_comp_{method}",
        flags="f",
    )


def test_nonexistent_source_project_raises(temp_region):
    """Referencing a non-existent source project must fail cleanly."""
    gs.run_command("g.region", n=50, s=0, e=50, w=0, res=1)
    gs.run_command(
        "r.mapcalc", expression="test_bad_proj_in = 1", overwrite=True
    )

    with pytest.raises(CalledModuleError):
        gs.run_command(
            "r.proj",
            input="test_bad_proj_in",
            project="this_project_does_not_exist_anywhere",
            output="test_bad_proj_out",
        )

    gs.run_command(
        "g.remove", type="raster", name="test_bad_proj_in", flags="f"
    )
