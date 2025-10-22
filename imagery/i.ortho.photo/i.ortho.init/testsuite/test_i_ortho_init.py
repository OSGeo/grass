import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestIOrthoInit(TestCase):
    """Regression tests for the i.ortho.init module"""

    test_group = "test_ortho_init_group"
    test_raster = "test_ortho_init_raster"

    test_xc = 500000.123
    test_yc = 4500000.456
    test_zc = 1500.789
    test_omega = 2.5
    test_phi = -1.8
    test_kappa = 45.2

    test_xc_sd = 10.0
    test_yc_sd = 10.0
    test_zc_sd = 5.0
    test_omega_sd = 2.0
    test_phi_sd = 2.0
    test_kappa_sd = 2.0

    @classmethod
    def setUpClass(cls):
        """Set up a temporary region and create test raster and group"""
        cls.use_temp_region()
        cls.runModule("g.region", n=10, s=0, e=10, w=0, rows=10, cols=10)
        cls.runModule(
            "r.mapcalc", expression=f"{cls.test_raster} = row()", overwrite=True
        )
        cls.runModule("i.group", group=cls.test_group, input=cls.test_raster)

    @classmethod
    def tearDownClass(cls):
        """Remove all temporary data and region"""
        gs.run_command("g.remove", flags="f", type="raster", name=cls.test_raster)
        gs.run_command("g.remove", flags="f", type="group", name=cls.test_group)
        cls.del_temp_region()

    def _run_init_module(self, flags="", **overrides):
        """Helper to run i.ortho.init with default parameters and optional overrides"""
        params = {
            "group": self.test_group,
            "xc": self.test_xc,
            "yc": self.test_yc,
            "zc": self.test_zc,
            "xc_sd": self.test_xc_sd,
            "yc_sd": self.test_yc_sd,
            "zc_sd": self.test_zc_sd,
            "omega": self.test_omega,
            "phi": self.test_phi,
            "kappa": self.test_kappa,
            "omega_sd": self.test_omega_sd,
            "phi_sd": self.test_phi_sd,
            "kappa_sd": self.test_kappa_sd,
        }
        params.update(overrides)
        self.assertModule("i.ortho.init", flags=flags, **params)

    def _assert_camera_params_equal(self, output_dict, expected_dict):
        """Helper to assert all expected camera parameters with 3 decimal tolerance"""
        for key, expected in expected_dict.items():
            actual = float(output_dict[key])
            self.assertAlmostEqual(
                actual,
                expected,
                places=3,
                msg=f"{key} mismatch: got {actual}, expected {expected}",
            )

    def test_create_camera_model_with_use_flag(self):
        """Test full camera model creation with use flag (-r)"""
        self._run_init_module()
        output = gs.parse_command("i.ortho.init", group=self.test_group, flags="p")

        expected_values = {
            "xc": self.test_xc,
            "yc": self.test_yc,
            "zc": self.test_zc,
            "omega": self.test_omega,
            "phi": self.test_phi,
            "kappa": self.test_kappa,
            "xc_sd": self.test_xc_sd,
            "yc_sd": self.test_yc_sd,
            "zc_sd": self.test_zc_sd,
            "omega_sd": self.test_omega_sd,
            "phi_sd": self.test_phi_sd,
            "kappa_sd": self.test_kappa_sd,
        }

        self._assert_camera_params_equal(output, expected_values)

    def test_partial_update_of_parameters(self):
        """Test that selected camera parameters can be updated after initial creation"""
        self._run_init_module()

        updated_values = {"omega": 5.7, "kappa": -30.5, "zc_sd": 15.0}

        self.assertModule("i.ortho.init", group=self.test_group, **updated_values)
        output = gs.parse_command("i.ortho.init", group=self.test_group, flags="p")

        expected_values = {
            "omega": updated_values["omega"],
            "kappa": updated_values["kappa"],
            "zc_sd": updated_values["zc_sd"],
            "xc": self.test_xc,
            "phi": self.test_phi,
        }

        self._assert_camera_params_equal(output, expected_values)


if __name__ == "__main__":
    test()
