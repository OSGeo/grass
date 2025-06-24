import os
import tempfile
from pathlib import Path
from grass.gunittest.case import TestCase
import grass.script as gs
from grass.gunittest.main import test


class TestIRectify(TestCase):
    """Regression testsuite for i.rectify module."""

    input_maps = ["rectify_band1", "rectify_band2"]
    group = "test_rectify_group"
    tmp_rasters = []
    tmp_files = []

    GCP_SETS = {
        "affine": """\
    1 1 1 1 1 1
    9 1 9 1 1 1
    1 9 1 9 1 1
    9 9 9 9 1 1
    5 5 5 5 1 1
    """,
        "order2": """\
    1 1 1 1 1 1
    1 5 1 5 1 1
    1 9 1 9 1 1
    3 2 3 2 1 1
    3 7 3 7 1 1
    5 5 5 5 1 1
    """,
        "order3": """\
    1 1 1 1 1 1
    1 5 1 5 1 1
    1 9 1 9 1 1
    3 2 3 2 1 1
    3 7 3 7 1 1
    5 1 5 1 1 1
    5 5 5 5 1 1
    5 9 5 9 1 1
    7 2 7 2 1 1
    7 8 7 8 1 1
    9 1 9 1 1 1
    9 9 9 9 1 1
    """,
    }

    @classmethod
    def setUpClass(cls):
        """Set up input rasters, group, and target."""
        cls.use_temp_region()
        cls.runModule("g.region", n=10, s=0, e=10, w=0, res=1)

        expressions = [
            f"{cls.input_maps[0]} = row() + col()",
            f"{cls.input_maps[1]} = row() * col()",
        ]
        for expr in expressions:
            cls.runModule("r.mapcalc", expression=expr, overwrite=True)

        cls.tmp_rasters.extend(cls.input_maps)
        cls.runModule("i.group", group=cls.group, input=",".join(cls.input_maps))

        env = gs.gisenv()
        cls.group_path = os.path.join(
            env["GISDBASE"], env["LOCATION_NAME"], env["MAPSET"], "group", cls.group
        )
        os.makedirs(cls.group_path, exist_ok=True)
        cls.points_path = os.path.join(cls.group_path, "POINTS")
        cls.runModule(
            "i.target",
            group=cls.group,
            location=env["LOCATION_NAME"],
            mapset=env["MAPSET"],
        )

    @classmethod
    def set_control_points(cls, gcp_string):
        """Helper to set control points for rectification."""
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as f:
            f.write(gcp_string)
            temp_path = f.name

        cls.tmp_files.append(temp_path)
        Path(temp_path).replace(cls.points_path)

    @classmethod
    def tearDownClass(cls):
        """Remove input rasters, group, and temporary files."""
        cls.runModule("g.remove", type="raster", name=cls.tmp_rasters, flags="f")
        cls.runModule("g.remove", type="group", name=cls.group, flags="f")
        if os.path.exists(cls.points_path):
            os.remove(cls.points_path)
        for f in cls.tmp_files:
            if os.path.exists(f):
                os.remove(f)

    def _run_and_check(
        self,
        extension,
        order,
        method,
        stats=None,
        flags=None,
        resolution=None,
        single_input=False,
        gcp_key=None,
    ):
        """Helper to run i.rectify and check the results."""
        if gcp_key:
            self.set_control_points(self.GCP_SETS[gcp_key])

        inputs = self.input_maps[:1] if single_input else self.input_maps
        args = {
            "group": self.group,
            "input": ",".join(inputs),
            "extension": extension,
            "order": order,
            "method": method,
        }
        if resolution is not None:
            args["resolution"] = resolution
        if flags:
            args["flags"] = flags

        self.assertModule("i.rectify", **args)

        for name in inputs:
            rectified = f"{name}{extension}"
            self.assertRasterExists(rectified)
            self.tmp_rasters.append(rectified)
            if stats:
                self.assertRasterFitsUnivar(rectified, stats[rectified], precision=1e-6)

    def test_affine_rectification(self):
        """Validate affine transformation (order=1) using cubic interpolation."""

        self._run_and_check(
            "order1_poly",
            1,
            "cubic",
            {
                "rectify_band1order1_poly": {"mean": 10.0, "max": 16.0, "min": 4.0},
                "rectify_band2order1_poly": {"mean": 25, "max": 64.0, "min": 4.0},
            },
            gcp_key="affine",
        )

    def test_polynomial_order2_rectification(self):
        """Validate polynomial transformation of order 2 using linear interpolation."""
        self._run_and_check(
            "order2_poly",
            2,
            "linear",
            {
                "rectify_band1order2_poly": {"mean": 10.0},
                "rectify_band2order2_poly": {"mean": 25.0},
            },
            gcp_key="order2",
        )

    def test_polynomial_order3_rectification(self):
        """Validate polynomial transformation of order 3 using lanczos interpolation."""
        self._run_and_check(
            "order3_poly",
            3,
            "lanczos",
            {
                "rectify_band1order3_poly": {"mean": 11.0},
                "rectify_band2order3_poly": {"mean": 30.25},
            },
            gcp_key="order3",
        )

    def test_region_restriction(self):
        """Test output restriction to current region using 'c' flag."""
        self.use_temp_region()
        self.runModule("g.region", n=5, s=0, e=5, w=0, rows=5, cols=5)
        self._run_and_check("cflag", 1, "nearest", flags="c", gcp_key="affine")
        for name in self.input_maps:
            rectified = f"{name}cflag"
            info = gs.parse_command("r.info", map=rectified, format="json")
            self.assertEqual(info["north"], 5.0)
            self.assertEqual(info["south"], 0.0)
            self.assertEqual(info["east"], 5.0)
            self.assertEqual(info["west"], 0.0)
        self.del_temp_region()

    def test_c_flag_with_resolution_override(self):
        """Test that 'c' flag overrides explicit resolution setting."""
        self.use_temp_region()
        self.runModule("g.region", n=5, s=0, e=5, w=0, res=5)
        self._run_and_check(
            "cflag_res", 1, "nearest", flags="c", resolution=2, gcp_key="affine"
        )
        for name in self.input_maps:
            rectified = f"{name}cflag_res"
            info = gs.parse_command("r.info", map=rectified, format="json")
            self.assertEqual(info["nsres"], 5.0)
            self.assertEqual(info["ewres"], 5.0)
        self.del_temp_region()

    def test_automatic_rectification(self):
        """Test automatic rectification of all maps in the group using 'a' flag."""
        self._run_and_check(
            "auto",
            1,
            "cubic_f",
            flags="a",
            stats={
                "rectify_band1auto": {"mean": 11.0, "max": 20.0, "min": 2.0},
                "rectify_band2auto": {"mean": 30.25, "max": 100.0, "min": 1.0},
            },
            gcp_key="affine",
        )

    def test_thin_plate_spline(self):
        """Test thin plate spline transformation using 't' flag."""
        self._run_and_check(
            "tps",
            1,
            "linear_f",
            flags="t",
            stats={
                "rectify_band1tps": {"mean": 11.0},
                "rectify_band2tps": {"mean": 30.25},
            },
            gcp_key="affine",
        )

    def test_resolution_parameter(self):
        """Test explicit resolution parameter for output raster."""
        self._run_and_check(
            "res2", 1, "lanczos_f", resolution=2, single_input=True, gcp_key="affine"
        )
        info = gs.parse_command("r.info", map="rectify_band1res2", format="json")
        self.assertEqual(info["nsres"], 2.0)
        self.assertEqual(info["ewres"], 2.0)


if __name__ == "__main__":
    test()
