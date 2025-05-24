import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.script import array
import numpy as np


class TestEvapotranspirationPT(TestCase):
    """Regression tests for i.evapo.pt module."""

    tmp_rasters = []
    net_radiation = "netrad"
    soil_heatflux = "soil_heatflux"
    air_temperature = "air_temperature"
    atmospheric_pressure = "atmospheric_pressure"
    priestley_taylor_coeff = 1.26
    output_map = "pt_output"

    @classmethod
    def setUpClass(cls):
        """Setup input rasters and configure test environment."""
        seed = 42
        cls.use_temp_region()
        gs.run_command("g.region", n=10, s=0, e=10, w=0, rows=10, cols=10)

        def generate_raster(name, expr_range):
            expr = f"{name} = rand({expr_range[0]}, {expr_range[1]})"
            cls.runModule("r.mapcalc", expression=expr, seed=seed, overwrite=True)
            cls.tmp_rasters.append(name)

        generate_raster(cls.net_radiation, (0, 160))
        generate_raster(cls.soil_heatflux, (0, 98))
        generate_raster(cls.air_temperature, (288, 303))
        generate_raster(cls.atmospheric_pressure, (1000, 1050))

    @classmethod
    def tearDownClass(cls):
        """Remove generated rasters and reset test environment."""
        cls.del_temp_region()
        cls.tmp_rasters.append(cls.output_map)
        gs.run_command("g.remove", type="raster", name=cls.tmp_rasters, flags="f")

    def run_evapo_pt(self, output, alpha=None, flags="", netrad=None, soilheat=None):
        """Helper to execute the i.evapo.pt module with default parameters."""
        return self.assertModule(
            "i.evapo.pt",
            net_radiation=netrad or self.net_radiation,
            soil_heatflux=soilheat or self.soil_heatflux,
            air_temperature=self.air_temperature,
            atmospheric_pressure=self.atmospheric_pressure,
            priestley_taylor_coeff=alpha or self.priestley_taylor_coeff,
            output=output,
            flags=flags,
            overwrite=True,
        )

    def test_z_flag(self):
        """Ensure -z flag clamps all negative ET values to zero."""
        self.run_evapo_pt(output=self.output_map, flags="z")
        stats = gs.parse_command(
            "r.univar", map=self.output_map, flags="g", format="json"
        )
        self.assertGreaterEqual(stats["min"], 0)

    def test_default_alpha(self):
        """Verify regression output with default alpha (1.26) against theoretical ET formula."""
        self.run_evapo_pt(output=self.output_map)

        net_rad = array.array(self.net_radiation)
        soil_heat = array.array(self.soil_heatflux)
        temp_k = array.array(self.air_temperature)
        patm = array.array(self.atmospheric_pressure)

        alpha = 1.26
        roh_w = 1004.15
        Cp = 1004.16

        temp_c = temp_k - 273.15
        b = temp_c + 237.3
        a = (17.27 * temp_c) / b

        delta = 2504.0 * np.exp(a) / (b**2)
        latent_term = 0.622 * 1e7 * (2.501 - 0.002361 * temp_c)
        ghamma = Cp * patm / latent_term
        latentHv = 86400.0 / ((2.501 - 0.002361 * temp_c) * 1e6)
        slope_ratio = delta / (delta + ghamma)

        expected_et = (alpha / roh_w) * slope_ratio * (net_rad - soil_heat) / latentHv

        expected_stats = {
            "mean": float(np.mean(expected_et)),
            "stddev": float(np.std(expected_et)),
            "min": float(np.min(expected_et)),
            "max": float(np.max(expected_et)),
        }

        self.assertRasterFitsUnivar(
            raster=self.output_map,
            reference=expected_stats,
            precision=1e-6,
        )

    def test_energy_scaling_invariance(self):
        """Check that doubling net radiation and soil heat flux scales ET output proportionally."""
        scale = 2.0
        scaled_netrad = "scaled_netrad"
        scaled_soilheat = "scaled_soilheat"
        scaled_output = "scaled_et"

        self.runModule(
            "r.mapcalc",
            expression=f"{scaled_netrad} = {self.net_radiation} * {scale}",
            overwrite=True,
        )
        self.runModule(
            "r.mapcalc",
            expression=f"{scaled_soilheat} = {self.soil_heatflux} * {scale}",
            overwrite=True,
        )
        self.tmp_rasters.extend([scaled_netrad, scaled_soilheat, scaled_output])

        self.run_evapo_pt(
            output=scaled_output, netrad=scaled_netrad, soilheat=scaled_soilheat
        )

        et_base = array.array(self.output_map)
        et_scaled = array.array(scaled_output)

        with np.errstate(divide="ignore", invalid="ignore"):
            ratio = np.true_divide(et_scaled, et_base)
            valid = np.isfinite(ratio) & (et_base != 0)

        self.assertTrue(
            np.allclose(ratio[valid], scale, rtol=1e-6, atol=1e-8),
            msg="ET output did not scale proportionally with energy input",
        )

    def test_zero_et_when_rnet_equals_soilheat(self):
        """Confirm ET output is zero when net radiation equals soil heat flux (no energy available)."""
        equal_input = "equal_energy"
        self.runModule(
            "r.mapcalc",
            expression=f"{equal_input} = rand(50, 100)",
            seed=99,
            overwrite=True,
        )
        self.tmp_rasters.append(equal_input)

        self.run_evapo_pt(
            output=self.output_map, netrad=equal_input, soilheat=equal_input
        )

        expected_stats = {"mean": 0.0, "stddev": 0.0, "min": 0.0, "max": 0.0}

        self.assertRasterFitsUnivar(self.output_map, expected_stats, precision=1e-6)


if __name__ == "__main__":
    test()
