import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestHSEBAL01(TestCase):
    """Regression testsuite for i.eb.hsebal01 module."""

    netradiation_map = "hsebal_netrad"
    soilheat_flux_map = "hsebal_soilheat"
    aerodynamic_resistance_map = "hsebal_z0m"
    temp_mean_sea_level_map = "hsebal_temp"
    vapor_pressure_map = "hsebal_vap"
    output_map = "hsebal_output"
    temp_rasters = []

    @classmethod
    def setUpClass(cls):
        """Set up the test environment and create input rasters."""
        cls.use_temp_region()
        cls.runModule("g.region", n=10000, s=0, e=10000, w=0, res=1000)

        rasters = [
            (cls.netradiation_map, "500 + col()"),
            (cls.soilheat_flux_map, "50 + col()"),
            (cls.aerodynamic_resistance_map, "0.1"),
            (cls.temp_mean_sea_level_map, "250 + row()*5 + col()*2"),
            (cls.vapor_pressure_map, "2 + row()/10.0"),
        ]
        for name, expr in rasters:
            cls.runModule("r.mapcalc", expression=f"{name} = {expr}", overwrite=True)

        cls.temp_rasters.extend([r[0] for r in rasters])

    @classmethod
    def tearDownClass(cls):
        """Clean up the test environment by removing created rasters."""
        cls.runModule("g.remove", flags="f", type="raster", name=cls.temp_rasters)
        cls.del_temp_region()

    def _run_hsebal(self, output, friction_velocity=0.32407, flags="", **kwargs):
        """Helper to run i.eb.hsebal01 with given parameters."""
        self.assertModule(
            "i.eb.hsebal01",
            netradiation=self.netradiation_map,
            soilheatflux=self.soilheat_flux_map,
            aerodynresistance=self.aerodynamic_resistance_map,
            temperaturemeansealevel=self.temp_mean_sea_level_map,
            vapourpressureactual=self.vapor_pressure_map,
            frictionvelocitystar=friction_velocity,
            output=output,
            flags=flags,
            overwrite=True,
            **kwargs,
        )
        self.temp_rasters.append(output)
        self.assertRasterExists(output)

    def _assert_univar(self, raster, expected_stats, precision=1e-6):
        """Helper to assert raster statistics."""
        self.assertRasterFitsUnivar(
            raster,
            expected_stats,
            precision=precision,
        )

    def _create_raster(self, expr, output):
        """Helper to create a raster."""
        self.runModule(
            "r.mapcalc",
            expression=f"{output} = {expr}",
            overwrite=True,
        )
        self.temp_rasters.append(output)

    def test_hsebal01_auto_pixels(self):
        """Test i.eb.hsebal01 with automatic wet/dry pixel detection."""
        out = f"{self.output_map}_auto"
        self._run_hsebal(output=out, flags="a")

        expected_stats = {
            "min": 0.0,
            "max": 444.971643,
            "mean": 240.955903,
            "stddev": 108.761122,
        }
        self._assert_univar(out, expected_stats)

    def test_hsebal01_manual_pixels_rowcol(self):
        """Test i.eb.hsebal01 with manual wet/dry pixel (row/column)."""
        out = f"{self.output_map}_manual"
        self._run_hsebal(
            output=out,
            row_wet_pixel=0,
            column_wet_pixel=0,
            row_dry_pixel=9,
            column_dry_pixel=9,
        )

        expected_stats = {
            "min": 0.0,
            "max": 444.971643,
            "mean": 240.955903,
            "stddev": 108.761122,
        }
        self._assert_univar(out, expected_stats)

    def test_hsebal01_manual_pixels_coordinates(self):
        """Test i.eb.hsebal01 with manual wet/dry pixel (coordinates)."""
        out = f"{self.output_map}_manual_coords"
        self._run_hsebal(
            output=out,
            flags="c",
            row_wet_pixel=10000,
            column_wet_pixel=0,
            row_dry_pixel=1000,
            column_dry_pixel=9000,
        )

        expected_stats = {
            "min": 0.0,
            "max": 444.971643,
            "mean": 240.955903,
            "stddev": 108.761122,
        }
        self._assert_univar(out, expected_stats)

    def test_hsebal01_with_different_friction_velocity(self):
        """Test the sensitivity of i.eb.hsebal01 to changes in friction velocity."""
        out1 = f"{self.output_map}_diff_friction"
        out2 = f"{self.output_map}_auto"

        self._run_hsebal(output=out1, flags="a", friction_velocity=0.5)
        self._run_hsebal(output=out2, flags="a", friction_velocity=0.32407)

        stats1 = gs.parse_command("r.univar", map=out1, format="json")
        stats2 = gs.parse_command("r.univar", map=out2, format="json")
        self.assertNotEqual(
            stats1["mean"],
            stats2["mean"],
            msg="Mean values should differ with different friction velocity star",
        )

    def test_hsebal01_energy_balance_constraint(self):
        """Test that sensible heat flux does not greatly exceed the available energy (Rn - G)."""
        out = f"{self.output_map}_energy_check"
        self._run_hsebal(output=out, flags="a")

        avail = f"{self.output_map}_available_energy"
        self._create_raster(
            f"{self.netradiation_map} - {self.soilheat_flux_map}", avail
        )

        mask = f"{self.output_map}_violation"
        self._create_raster(f"if({out} > {avail}, 1, 0)", mask)

        stats = gs.parse_command("r.univar", map=mask, format="json")
        ratio = stats["sum"] / stats["n"]
        self.assertLess(
            ratio,
            0.1,
            msg="Sensible heat flux should not greatly exceed available energy",
        )

    def test_hsebal01_monotonicity_along_temperature(self):
        """Test that higher temperatures generally result in higher sensible heat flux."""
        out = f"{self.output_map}_monotonicity"
        self._run_hsebal(output=out, flags="a")

        low = f"{out}_low_temp"
        high = f"{out}_high_temp"
        self._create_raster(f"if(row() < 5, {out}, null())", low)
        self._create_raster(f"if(row() >= 5, {out}, null())", high)

        stats_low = gs.parse_command("r.univar", map=low, format="json")
        stats_high = gs.parse_command("r.univar", map=high, format="json")

        mean_low = stats_low["mean"]
        mean_high = stats_high["mean"]

        self.assertGreater(
            mean_high,
            mean_low,
            msg="Higher temperature area does not have higher mean sensible heat flux",
        )


if __name__ == "__main__":
    test()
