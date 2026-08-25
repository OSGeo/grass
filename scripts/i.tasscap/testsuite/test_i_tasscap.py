import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestITasscap(TestCase):
    """Regression testsuite for i.tasscap GRASS module."""

    input_rasters = []
    output_rasters = []

    @classmethod
    def setUpClass(cls):
        """Create constant rasters for all supported sensors."""
        cls.landsat_bands = [f"ls_band{i}" for i in range(1, 7)]
        cls.modis_bands = [f"modis_band{i}" for i in range(1, 8)]
        cls.sentinel_bands = [f"sentinel_band{i}" for i in range(1, 14)]
        cls.worldview_bands = [f"wv_band{i}" for i in range(1, 9)]

        cls.input_rasters = (
            cls.landsat_bands
            + cls.modis_bands
            + cls.sentinel_bands
            + cls.worldview_bands
        )

        for b in cls.input_rasters:
            cls.runModule("r.mapcalc", expression=f"{b}=10", overwrite=True)

    @classmethod
    def tearDownClass(cls):
        """Remove all synthetic raster maps."""
        cls.runModule(
            "g.remove",
            flags="f",
            type="raster",
            name=cls.input_rasters + cls.output_rasters,
        )

    def _assert_component_outputs(self, output_base, num_components, expected_sums):
        """
        Helper to assert that outputs exist, have correct metadata, and correct numerical values.
        """
        for i in range(1, num_components + 1):
            name = f"{output_base}.{i}"

            self.assertRasterExists(name)
            self.output_rasters.append(name)

            desc = gs.read_command("r.info", map=name, flags="e")
            self.assertIn("Tasseled Cap", desc)

            stats = gs.parse_command("r.univar", map=name, format="json")
            mean = stats["mean"]
            self.assertAlmostEqual(mean, expected_sums[i - 1], places=4)

    def test_landsat5_tm(self):
        """Test most commonly used Landsat sensor with mathematical validation."""
        output = "tasscap_ls5"
        self.runModule(
            "i.tasscap",
            input=",".join(self.landsat_bands),
            output=output,
            sensor="landsat5_tm",
            overwrite=True,
        )

        weights = [
            sum(10 * w for w in row[:-1]) + row[-1]
            for row in [
                (0.2909, 0.2493, 0.4806, 0.5568, 0.4438, 0.1706, 10.3695),
                (-0.2728, -0.2174, -0.5508, 0.7221, 0.0733, -0.1648, -0.7310),
                (0.1446, 0.1761, 0.3322, 0.3396, -0.6210, -0.4186, -3.3828),
                (0.8461, -0.0731, -0.4640, -0.0032, -0.0492, -0.0119, 0.7879),
            ]
        ]

        self._assert_component_outputs(output, num_components=4, expected_sums=weights)

    def test_modis(self):
        """Validate MODIS transformation (7-band sensor without constants)."""
        output = "tasscap_modis"
        self.runModule(
            "i.tasscap",
            input=",".join(self.modis_bands),
            output=output,
            sensor="modis",
            overwrite=True,
        )

        weights = [
            sum(10 * w for w in row)
            for row in [
                (0.4395, 0.5945, 0.2460, 0.3918, 0.3506, 0.2136, 0.2678),
                (-0.4064, 0.5129, -0.2744, -0.2893, 0.4882, -0.0036, -0.4169),
                (0.1147, 0.2489, 0.2408, 0.3132, -0.3122, -0.6416, -0.5087),
            ]
        ]

        self._assert_component_outputs(output, num_components=3, expected_sums=weights)

    def test_sentinel2(self):
        """Validate Sentinel-2 transformation (13 bands)."""
        output = "tasscap_sentinel"
        self.runModule(
            "i.tasscap",
            input=",".join(self.sentinel_bands),
            output=output,
            sensor="sentinel2",
            overwrite=True,
        )

        weights = [
            sum(10 * w for w in row)
            for row in [
                (
                    0.0356,
                    0.0822,
                    0.1360,
                    0.2611,
                    0.2964,
                    0.3338,
                    0.3877,
                    0.3895,
                    0.0949,
                    0.0009,
                    0.3882,
                    0.1366,
                    0.4750,
                ),
                (
                    -0.0635,
                    -0.1128,
                    -0.1680,
                    -0.3480,
                    -0.3303,
                    0.0852,
                    0.3302,
                    0.3165,
                    0.0467,
                    -0.0009,
                    -0.4578,
                    -0.4064,
                    0.3625,
                ),
                (
                    0.0649,
                    0.1363,
                    0.2802,
                    0.3072,
                    0.5288,
                    0.1379,
                    -0.0001,
                    -0.0807,
                    -0.0302,
                    0.0003,
                    -0.4064,
                    -0.5602,
                    -0.1389,
                ),
            ]
        ]

        self._assert_component_outputs(output, num_components=3, expected_sums=weights)

    def test_component_count_validation(self):
        """Ensure each sensor produces the correct number of component outputs."""
        sensors = [
            ("landsat4_tm", self.landsat_bands, 3),
            ("landsat5_tm", self.landsat_bands, 4),
            ("modis", self.modis_bands, 3),
            ("worldview2", self.worldview_bands, 4),
        ]

        for sensor, bands, expected_count in sensors:
            output = f"test_{sensor}"
            self.runModule(
                "i.tasscap",
                input=",".join(bands),
                output=output,
                sensor=sensor,
                overwrite=True,
            )

            for i in range(1, expected_count + 1):
                self.assertRasterExists(f"{output}.{i}")
                self.output_rasters.extend(
                    [f"{output}.{i}" for i in range(1, expected_count + 1)]
                )


if __name__ == "__main__":
    test()
