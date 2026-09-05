import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestR3GWFlow(TestCase):
    """Regression tests for the r3.gwflow GRASS module."""

    tmp_3d_rasters = []

    @classmethod
    def setUpClass(cls):
        """Set up the test environment and create input raster maps."""
        cls.use_temp_region()
        cls.runModule(
            "g.region", res=25, res3=25, t=75, b=0, n=100, s=0, w=0, e=100, flags="p3"
        )

        cls.runModule(
            "r3.mapcalc",
            expression="phead = if(row() == 1 && depth() == 2, 50, 40)",
            overwrite=True,
        )
        cls.runModule(
            "r3.mapcalc",
            expression="status = if(row() == 1 && depth() == 2, 2, 1)",
            overwrite=True,
        )
        cls.runModule(
            "r3.mapcalc",
            expression="well = if(row() == 2 && col() == 2 && depth() == 1, -0.25, 0)",
            overwrite=True,
        )
        cls.runModule("r3.mapcalc", expression="hydcond = 0.00025", overwrite=True)
        cls.runModule("r3.mapcalc", expression="syield = 0.0001", overwrite=True)
        cls.runModule("r3.mapcalc", expression="recharge = 0.0", overwrite=True)
        cls.tmp_3d_rasters.extend(
            ["phead", "status", "well", "hydcond", "syield", "recharge"]
        )

        cls.runModule(
            "r3.mapcalc",
            expression="status_mixed = if(row() == 1, 2, if(col() > 2, 1, 0))",
            overwrite=True,
        )
        cls.tmp_3d_rasters.append("status_mixed")
        cls.runModule(
            "r3.mapcalc",
            expression="hydcond_variable = 0.0001 + (col() * 0.0001)",
            overwrite=True,
        )
        cls.tmp_3d_rasters.append("hydcond_variable")

    @classmethod
    def tearDownClass(cls):
        """Clean up the test environment by removing created raster maps."""
        cls.runModule("g.remove", flags="f", type="raster_3d", name=cls.tmp_3d_rasters)
        cls.del_temp_region()

    def _run_gwflow(
        self,
        suffix,
        phead="phead",
        status="status",
        hc_x="hydcond",
        hc_y="hydcond",
        hc_z="hydcond",
        yield_="syield",
        output=None,
        **kwargs,
    ):
        """
        Helper function to run r3.gwflow with flexible parameters.
        """
        if output is None:
            output = f"gw_out_{suffix}"

        module_params = {
            "phead": phead,
            "status": status,
            "hc_x": hc_x,
            "hc_y": hc_y,
            "hc_z": hc_z,
            "yield": yield_,
            "output": output,
            "dtime": kwargs.get("dtime", 86400),
            "maxit": kwargs.get("maxit", 500),
            "error": kwargs.get("error", 1e-6),
            "solver": kwargs.get("solver", "cg"),
        }

        optional_params = [
            "sink",
            "recharge",
            "velocity_x",
            "velocity_y",
            "velocity_z",
            "budget",
        ]
        for param in optional_params:
            if param in kwargs:
                module_params[param] = kwargs[param]

        if "velocity_x" not in kwargs:
            module_params["velocity_x"] = f"vx_{suffix}"
        if "velocity_y" not in kwargs:
            module_params["velocity_y"] = f"vy_{suffix}"
        if "velocity_z" not in kwargs:
            module_params["velocity_z"] = f"vz_{suffix}"
        if "budget" not in kwargs:
            module_params["budget"] = f"budget_{suffix}"

        if "flags" in kwargs:
            module_params["flags"] = kwargs["flags"]

        self.assertModule("r3.gwflow", **module_params)
        self.assertRaster3dExists(output)

        self.tmp_3d_rasters.extend(
            [
                module_params["budget"],
                module_params["velocity_x"],
                module_params["velocity_y"],
                module_params["velocity_z"],
                output,
            ]
        )

    def test_gwflow_default_solver(self):
        """Run basic simulation using default 'cg' solver and verify univariate statistics."""
        self._run_gwflow("default", sink="well", recharge="recharge")
        expected_stats = {
            "min": 34.347307,
            "max": 50.0,
            "mean": 41.435054,
            "stddev": 2.914188,
        }
        self.assertRaster3dFitsUnivar("gw_out_default", expected_stats, precision=1e-6)

    def test_full_matrix_matches_sparse_close(self):
        """Compare full matrix solver (-f) against sparse solver (cg) for numeric consistency."""
        self._run_gwflow("cg_compare", solver="cg", sink="well", recharge="recharge")
        self._run_gwflow(
            "f_compare", solver="cholesky", flags="f", sink="well", recharge="recharge"
        )

        self.runModule(
            "r3.mapcalc",
            expression=("diffmap = abs(gw_out_f_compare - gw_out_cg_compare)"),
            overwrite=True,
        )
        stats = gs.parse_command("r3.univar", map="diffmap", flags="g")
        self.tmp_3d_rasters.append("diffmap")
        self.assertLess(float(stats["max"]), 1e-2)

    def test_gwflow_mixed_boundary_conditions(self):
        """Validate solution under mixed boundary conditions (Dirichlet, Neumann, inactive)."""
        self._run_gwflow(
            "mixed", status="status_mixed", sink="well", recharge="recharge"
        )
        stats = gs.parse_command("r3.univar", map="gw_out_mixed", flags="g")
        self.assertIsNotNone(float(stats["n"]))
        expected_stats = {
            "min": 40.0,
            "max": 50.0,
            "mean": 41.597213,
            "stddev": 3.328858,
        }
        self.assertRaster3dFitsUnivar("gw_out_mixed", expected_stats, precision=1e-6)

    def test_gwflow_variable_hydraulic_conductivity(self):
        """Check that spatially variable conductivity leads to higher velocity variation."""
        self._run_gwflow("const_hc", sink="well", recharge="recharge")
        self._run_gwflow(
            "variable_hc",
            hc_x="hydcond_variable",
            hc_y="hydcond_variable",
            hc_z="hydcond_variable",
            sink="well",
            recharge="recharge",
        )
        vstats_const = gs.parse_command("r3.univar", map="vx_const_hc", flags="g")
        vstats_var = gs.parse_command("r3.univar", map="vx_variable_hc", flags="g")
        self.assertGreater(float(vstats_var["stddev"]), float(vstats_const["stddev"]))


if __name__ == "__main__":
    test()
