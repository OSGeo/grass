"""Basic unit tests for r.proj.

Every test requests ``temp_region`` which pulls in ``grass_session``
automatically.  That fixture (in conftest.py) creates a temporary project
and calls gs.setup.init so that all GRASS modules work in CI without a
pre-existing GRASS session.
"""

import pytest
import grass.script as gs
from grass.exceptions import CalledModuleError


# ---------------------------------------------------------------------------
# helper
# ---------------------------------------------------------------------------
def _make_raster(name, expression="row() * 10 + col()"):
    """Create a small test raster in the current region."""
    gs.run_command("r.mapcalc", expression=f"{name} = {expression}", overwrite=True)


# ---------------------------------------------------------------------------
# identity / same-CRS tests
# ---------------------------------------------------------------------------
class TestRProjBasic:
    """Tests that stay inside a single project."""

    def test_same_projection_identity_transform(self, temp_region):
        """Reprojecting within the same CRS should preserve values."""
        gs.run_command("g.region", n=50, s=0, e=50, w=0, res=1)
        _make_raster("test_identity_in")

        gs.run_command(
            "r.proj",
            input="test_identity_in",
            output="test_identity_out",
            method="nearest",
            overwrite=True,
        )

        assert gs.find_file("test_identity_out", element="raster")["name"]

        in_stats = gs.parse_command("r.univar", map="test_identity_in", flags="g")
        out_stats = gs.parse_command("r.univar", map="test_identity_out", flags="g")
        assert float(in_stats["mean"]) == pytest.approx(
            float(out_stats["mean"]), rel=1e-3
        )

        gs.run_command(
            "g.remove",
            type="raster",
            name="test_identity_in,test_identity_out",
            flags="f",
        )

    def test_output_raster_exists(self, temp_region):
        """r.proj must produce an output raster."""
        gs.run_command("g.region", n=50, s=0, e=50, w=0, res=1)
        _make_raster("test_exists_in")

        gs.run_command(
            "r.proj",
            input="test_exists_in",
            output="test_exists_out",
            method="nearest",
            overwrite=True,
        )

        assert gs.find_file("test_exists_out", element="raster")["name"]
        gs.run_command(
            "g.remove",
            type="raster",
            name="test_exists_in,test_exists_out",
            flags="f",
        )

    def test_region_not_silently_changed(self, temp_region):
        """r.proj must not silently alter the computational region."""
        gs.run_command("g.region", n=100, s=0, e=100, w=0, res=2)
        before = gs.parse_command("g.region", flags="gp")

        _make_raster("test_region_in")
        gs.run_command(
            "r.proj",
            input="test_region_in",
            output="test_region_out",
            method="nearest",
            overwrite=True,
        )

        after = gs.parse_command("g.region", flags="gp")
        assert before["n"] == after["n"]
        assert before["s"] == after["s"]

        gs.run_command(
            "g.remove",
            type="raster",
            name="test_region_in,test_region_out",
            flags="f",
        )


# ---------------------------------------------------------------------------
# interpolation
# ---------------------------------------------------------------------------
class TestRProjInterpolation:
    """Every supported interpolation keyword must be accepted."""

    @pytest.mark.parametrize("method", ["nearest", "bilinear", "bicubic"])
    def test_interpolation_method_runs(self, temp_region, method):
        gs.run_command("g.region", n=50, s=0, e=50, w=0, res=1)
        _make_raster("test_interp_in")

        gs.run_command(
            "r.proj",
            input="test_interp_in",
            output=f"test_interp_{method}",
            method=method,
            overwrite=True,
        )

        assert gs.find_file(f"test_interp_{method}", element="raster")["name"]
        gs.run_command(
            "g.remove",
            type="raster",
            name=f"test_interp_in,test_interp_{method}",
            flags="f",
        )

    def test_nearest_preserves_values(self, temp_region):
        """Nearest-neighbor must not interpolate – mean stays the same."""
        gs.run_command("g.region", n=50, s=0, e=50, w=0, res=1)
        _make_raster("test_nn_in")

        gs.run_command(
            "r.proj",
            input="test_nn_in",
            output="test_nn_out",
            method="nearest",
            overwrite=True,
        )

        in_stats = gs.parse_command("r.univar", map="test_nn_in", flags="g")
        out_stats = gs.parse_command("r.univar", map="test_nn_out", flags="g")
        assert float(in_stats["mean"]) == pytest.approx(
            float(out_stats["mean"]), rel=1e-3
        )

        gs.run_command(
            "g.remove", type="raster", name="test_nn_in,test_nn_out", flags="f"
        )


# ---------------------------------------------------------------------------
# error handling
# ---------------------------------------------------------------------------
class TestRProjInvalidParams:
    """r.proj must reject bad parameters gracefully."""

    def test_invalid_project_raises(self, temp_region):
        """Non-existent source project must raise CalledModuleError."""
        gs.run_command("g.region", n=50, s=0, e=50, w=0, res=1)
        _make_raster("test_inv_proj_in")

        with pytest.raises(CalledModuleError):
            gs.run_command(
                "r.proj",
                input="test_inv_proj_in",
                project="this_project_does_not_exist_xyz",
                output="test_inv_proj_out",
                method="nearest",
            )

        gs.run_command(
            "g.remove", type="raster", name="test_inv_proj_in", flags="f"
        )

    @pytest.mark.parametrize(
        "bad_param",
        [
            {"method": "invalid_method_xyz"},
            {"memory": "-999"},
        ],
    )
    def test_invalid_parameters(self, temp_region, bad_param):
        gs.run_command("g.region", n=50, s=0, e=50, w=0, res=1)
        _make_raster("test_bad_in")

        with pytest.raises(CalledModuleError):
            gs.run_command(
                "r.proj",
                input="test_bad_in",
                output="test_bad_out",
                **bad_param,
            )

        gs.run_command(
            "g.remove", type="raster", name="test_bad_in", flags="f"
        )
