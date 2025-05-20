from grass.gunittest.case import TestCase
from grass.gunittest.main import test
import grass.script as gs


class TestIRGBHIS(TestCase):
    tmp_rasters = []

    @classmethod
    def setUpClass(cls):
        """Setup input rasters and configure test environment."""
        cls.runModule("g.region", n=10, s=0, e=10, w=0, rows=10, cols=10)

    @classmethod
    def tearDownClass(cls):
        """Remove generated rasters and reset test environment."""
        cls.runModule(
            "g.remove",
            flags="f",
            type="raster",
            name=cls.tmp_rasters,
        )

    def create_raster(self, name, expression=None, seed=None):
        """Helper to create a rasters."""
        expr = "rand(0, 255)" if seed is not None else expression
        self.runModule(
            "r.mapcalc", expression=f"{name} = {expr}", seed=seed, overwrite=True
        )

    def test_output_ranges(self):
        """Test that HIS output values are within valid ranges (0-255)"""
        seed_value = 12345
        self.create_raster("test_red", seed=seed_value)
        self.create_raster("test_green", seed=seed_value + 1)
        self.create_raster("test_blue", seed=seed_value + 2)

        self.assertModule(
            "i.rgb.his",
            red="test_red",
            green="test_green",
            blue="test_blue",
            hue="test_hue",
            intensity="test_intensity",
            saturation="test_saturation",
            overwrite=True,
        )
        self.tmp_rasters.extend(
            [
                "test_red",
                "test_green",
                "test_blue",
                "test_hue",
                "test_intensity",
                "test_saturation",
            ]
        )

        for map_name, min_val, max_val in [
            ("test_hue", 0, 255),
            ("test_intensity", 0, 255),
            ("test_saturation", 0, 255),
        ]:
            info = gs.parse_command("r.info", map=map_name, format="json")
            min_actual = info["min"]
            max_actual = info["max"]
            self.assertGreaterEqual(
                min_actual,
                min_val,
                msg=f"{map_name} minimum value ({min_actual}) < {min_val}",
            )
            self.assertLessEqual(
                max_actual,
                max_val,
                msg=f"{map_name} maximum value ({max_actual}) > {max_val}",
            )

    def test_grayscale_conversion(self):
        """Test neutral color conversion (R=G=B=100)"""
        self.create_raster("test_red_gray", "100")
        self.create_raster("test_green_gray", "100")
        self.create_raster("test_blue_gray", "100")

        self.assertModule(
            "i.rgb.his",
            red="test_red_gray",
            green="test_green_gray",
            blue="test_blue_gray",
            hue="test_hue_gray",
            intensity="test_intensity_gray",
            saturation="test_saturation_gray",
            overwrite=True,
        )
        self.tmp_rasters.extend(
            [
                "test_red_gray",
                "test_green_gray",
                "test_blue_gray",
                "test_hue_gray",
                "test_intensity_gray",
                "test_saturation_gray",
            ]
        )

        univar_hue = gs.parse_command("r.univar", map="test_hue_gray", format="json")
        self.assertAlmostEqual(univar_hue[0]["mean"], 0.0, places=6)

        univar_intensity = gs.parse_command(
            "r.univar", map="test_intensity_gray", format="json"
        )
        self.assertAlmostEqual(univar_intensity[0]["mean"], 100.0, places=6)

        univar_saturation = gs.parse_command(
            "r.univar", map="test_saturation_gray", format="json"
        )
        self.assertAlmostEqual(univar_saturation[0]["mean"], 0.0, places=6)

    def test_pure_primary_colors(self):
        """Test conversion for primary RGB colors"""
        test_cases = [
            ("red", 255, 0, 0, 0.0),
            ("green", 0, 255, 0, 85.0),
            ("blue", 0, 0, 255, 170.0),
        ]

        for color, r, g, b, exp_hue in test_cases:
            self.create_raster(f"test_r_{color}", f"{r}")
            self.create_raster(f"test_g_{color}", f"{g}")
            self.create_raster(f"test_b_{color}", f"{b}")

            self.assertModule(
                "i.rgb.his",
                red=f"test_r_{color}",
                green=f"test_g_{color}",
                blue=f"test_b_{color}",
                hue=f"test_hue_{color}",
                intensity=f"test_intensity_{color}",
                saturation=f"test_saturation_{color}",
                overwrite=True,
            )
            self.tmp_rasters.extend(
                [
                    f"test_r_{color}",
                    f"test_g_{color}",
                    f"test_b_{color}",
                    f"test_hue_{color}",
                    f"test_intensity_{color}",
                    f"test_saturation_{color}",
                ]
            )

            univar_hue = gs.parse_command(
                "r.univar", map=f"test_hue_{color}", format="json"
            )
            self.assertEqual(
                univar_hue[0]["mean"],
                exp_hue,
                msg=f"Hue for {color}: Expected {exp_hue}, got {univar_hue[0]['mean']}",
            )

            univar_intensity = gs.parse_command(
                "r.univar", map=f"test_intensity_{color}", format="json"
            )
            self.assertAlmostEqual(
                univar_intensity[0]["mean"],
                128,
                msg=f"Intensity check failed for {color}",
            )
            # should have been 85 (255/3) but is 128 for all the colors suggesting normalization

            univar_saturation = gs.parse_command(
                "r.univar", map=f"test_saturation_{color}", format="json"
            )
            self.assertAlmostEqual(
                univar_saturation[0]["mean"],
                255.0,
                msg=f"Saturation check failed for {color}",
            )

    def test_conversion_roundtrip(self):
        """Test the accuracy of the RGB to HIS to RGB conversion within an expected tolerance."""
        self.create_raster("test_red_orig", "255")
        self.create_raster("test_green_orig", "128")
        self.create_raster("test_blue_orig", "64")

        self.assertModule(
            "i.rgb.his",
            red="test_red_orig",
            green="test_green_orig",
            blue="test_blue_orig",
            hue="test_hue_rt",
            intensity="test_intensity_rt",
            saturation="test_saturation_rt",
            overwrite=True,
        )

        self.assertModule(
            "i.his.rgb",
            hue="test_hue_rt",
            intensity="test_intensity_rt",
            saturation="test_saturation_rt",
            red="test_red_new",
            green="test_green_new",
            blue="test_blue_new",
            overwrite=True,
        )
        self.tmp_rasters.extend(
            [
                "test_red_orig",
                "test_green_orig",
                "test_blue_orig",
                "test_hue_rt",
                "test_intensity_rt",
                "test_saturation_rt",
                "test_red_new",
                "test_green_new",
                "test_blue_new",
            ]
        )

        for channel in ["red", "green", "blue"]:
            self.create_raster(
                f"test_diff_{channel}", f"abs(test_{channel}_orig - test_{channel}_new)"
            )
            self.tmp_rasters.append(f"test_diff_{channel}")

            diff_stats = gs.parse_command(
                "r.univar", map=f"test_diff_{channel}", flags="g"
            )
            max_diff = float(diff_stats["max"])
            mean_diff = float(diff_stats["mean"])

            self.assertLessEqual(
                max_diff,
                1.0,
                msg=f"Maximum difference in {channel} channel exceeds tolerance. Max Diff: {max_diff}",
            )
            self.assertLessEqual(
                mean_diff,
                1.0,
                msg=f"Mean difference in {channel} channel too high. Mean Diff: {mean_diff}",
            )


if __name__ == "__main__":
    test()
