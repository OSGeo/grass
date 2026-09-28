import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestHISRGB(TestCase):
    """Regression tests for the i.his.rgb module."""

    tmp_rasters = []

    @classmethod
    def setUpClass(cls):
        """Set computational region and generate input rasters for tests."""
        cls.use_temp_region()
        gs.run_command("g.region", n=10, s=0, e=10, w=0, rows=10, cols=10)

        # Grayscale input (saturation = 0)
        gs.mapcalc("intensity_1 = 255", overwrite=True)
        gs.mapcalc("saturation_1 = 0", overwrite=True)
        gs.mapcalc("hue_1 = 0", overwrite=True)
        cls.tmp_rasters.extend(["intensity_1", "saturation_1", "hue_1"])

        # Primary hue inputs (saturation = 1, intensity = 255)
        gs.mapcalc("intensity_2 = 255", overwrite=True)
        gs.mapcalc("saturation_2 = 1", overwrite=True)
        gs.mapcalc("hue_red = 0", overwrite=True)
        gs.mapcalc("hue_green = 120", overwrite=True)
        gs.mapcalc("hue_blue = 240", overwrite=True)
        cls.tmp_rasters.extend(
            ["intensity_2", "saturation_2", "hue_red", "hue_green", "hue_blue"]
        )

    @classmethod
    def tearDownClass(cls):
        """Clean up temporary maps, region and mask."""
        cls.del_temp_region()
        gs.run_command("g.remove", type="raster", name=cls.tmp_rasters, flags="f")
        gs.run_command("r.mask", flags="r")

    def run_i_his_rgb(self, hue, intensity, saturation, red, green, blue):
        """Helper function to run i.his.rgb and register output raster maps for cleanup."""
        gs.run_command(
            "i.his.rgb",
            hue=hue,
            intensity=intensity,
            saturation=saturation,
            red=red,
            green=green,
            blue=blue,
            overwrite=True,
        )
        self.tmp_rasters.extend([red, green, blue])

    def test_grayscale(self):
        """Test output when saturation is 0; output should match intensity."""
        self.run_i_his_rgb(
            "hue_1",
            "intensity_1",
            "saturation_1",
            "red_test_1",
            "green_test_1",
            "blue_test_1",
        )
        # All three bands should be equal to the intensity input i.e. 255
        # but we get 0 due to incorrect condition in the conversion
        self.assertRasterMinMax("red_test_1", 255, 255)
        self.assertRasterMinMax("green_test_1", 255, 255)
        self.assertRasterMinMax("blue_test_1", 255, 255)

    def test_primary_colors(self):
        """Test that primary hues map to correct RGB components at full saturation/intensity."""

        # Red hue (0°)
        self.run_i_his_rgb(
            "hue_red",
            "intensity_2",
            "saturation_2",
            "red_test2",
            "green_test2",
            "blue_test2",
        )
        # Red should be 255, green and blue should be 0
        # but we get 0 due to incorrect handling
        self.assertRasterFitsUnivar("red_test2", {"min": 255, "max": 255})
        self.assertRasterFitsUnivar("green_test2", {"min": 0, "max": 0})
        self.assertRasterFitsUnivar("blue_test2", {"min": 0, "max": 0})

        # Green hue (120°)
        self.run_i_his_rgb(
            "hue_green",
            "intensity_2",
            "saturation_2",
            "red_test3",
            "green_test3",
            "blue_test3",
        )
        # Green should be 255, red and blue should be 0
        # but we get 0 due to incorrect handling
        self.assertRasterFitsUnivar("green_test3", {"min": 255, "max": 255})
        self.assertRasterFitsUnivar("red_test3", {"min": 0, "max": 0})
        self.assertRasterFitsUnivar("blue_test3", {"min": 0, "max": 0})

        # Blue hue (240°)
        self.run_i_his_rgb(
            "hue_blue",
            "intensity_2",
            "saturation_2",
            "red_test4",
            "green_test4",
            "blue_test4",
        )
        # Blue should be 255, red and green should be 0
        # but we get 0 due to incorrect handling
        self.assertRasterFitsUnivar("blue_test4", {"min": 255, "max": 255})
        self.assertRasterFitsUnivar("red_test4", {"min": 0, "max": 0})
        self.assertRasterFitsUnivar("green_test4", {"min": 0, "max": 0})

    def test_mask_handling(self):
        """Test that i.his.rgb respects the current raster mask and processes only unmasked areas."""
        gs.mapcalc(
            "mask_test = if(row() == 5 && col() == 5, 1, null())", overwrite=True
        )
        self.tmp_rasters.append("mask_test")

        gs.run_command("r.mask", raster="mask_test")

        self.run_i_his_rgb(
            "hue_1",
            "intensity_1",
            "saturation_1",
            "red_mask",
            "green_mask",
            "blue_mask",
        )

        for band in ["red_mask", "green_mask", "blue_mask"]:
            stats = gs.parse_command("r.univar", map=band, flags="g", format="json")[0]
            self.assertEqual(
                stats["n"], 1, msg=f"{band}: expected exactly one non-null cell"
            )
            # min and max should be 255 but we get 0
            # due to incorrect condition in the conversion
            self.assertEqual(stats["min"], 255, msg=f"{band}: expected value 255")
            self.assertEqual(stats["max"], 255, msg=f"{band}: expected value 255")
            self.assertEqual(
                stats["null_cells"], 99, msg=f"{band}: expected 99 null cells"
            )


if __name__ == "__main__":
    test()
