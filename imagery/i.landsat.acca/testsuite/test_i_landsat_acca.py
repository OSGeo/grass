import numpy as np
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.script import array


class TestILandsatAcca(TestCase):
    """Test case for the i.landsat.acca module."""

    input_basename = "B"
    output_cloud = "cloud_mask"

    @classmethod
    def setUpClass(cls):
        """Set up the region and input raster maps for testing."""
        cls.use_temp_region()
        cls.runModule("g.region", n=50, s=0, e=50, w=0, rows=100, cols=100)
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.input_basename}2 = if(row() < 50 && col() < 50, 0.4, 0.1)",
            overwrite=True,
        )
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.input_basename}3 = if(row() < 50 && col() < 50, 0.5, 0.15)",
            overwrite=True,
        )
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.input_basename}4 = if(row() < 50 && col() < 50, 0.6, 0.2)",
            overwrite=True,
        )
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.input_basename}5 = if(row() < 50 && col() < 50, 0.3, 0.1)",
            overwrite=True,
        )
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.input_basename}61 = if(row() < 50 && col() < 50, 270.0, 300.0)",
            overwrite=True,
        )

    @classmethod
    def tearDownClass(cls):
        """Remove temporary data and restore the region after testing."""
        cls.runModule(
            "g.remove",
            type="raster",
            name=f"{cls.input_basename}2,{cls.input_basename}3,{cls.input_basename}4,{cls.input_basename}5,{cls.input_basename}61,{cls.output_cloud}",
            flags="f",
            quiet=True,
        )
        cls.del_temp_region()

    def test_cloud_mask_and_thermal(self):
        """Test cloud mask creation and thermal correlation. The cloud mask is converted to binary (1 for clouds, 0 for non-clouds).
        The mask identifies 2401 pixels in a 49x49 region."""
        self.assertModule(
            "i.landsat.acca",
            input=self.input_basename,
            output=self.output_cloud,
            b45ratio=1.0,
            b56composite=225,
            overwrite=True,
        )
        self.runModule(
            "r.mapcalc",
            expression=f"{self.output_cloud} = if({self.output_cloud} > 0, 1, 0)",
            overwrite=True,
        )
        self.assertRasterExists(self.output_cloud)
        reference_stats = {
            "max": 1,
            "sum": 2401,
        }
        self.assertRasterFitsUnivar("cloud_mask", reference_stats, precision=1e-6)
        cloud_array = array.array(self.output_cloud)
        thermal_band = array.array(f"{self.input_basename}61")
        cloud_pixels = cloud_array > 0
        non_cloud_pixels = ~cloud_pixels
        avg_thermal_clouds = np.mean(thermal_band[cloud_pixels])
        avg_thermal_non_clouds = np.mean(thermal_band[non_cloud_pixels])
        self.assertGreater(
            avg_thermal_non_clouds,
            avg_thermal_clouds,
            "Non-cloud pixels should have higher thermal values than cloud pixels",
        )

    def test_cloud_shadow_detection(self):
        """Test the cloud shadow detection to verify it identifies shadow pixels."""
        shadow_output = "cloud_shadow_test"
        self.assertModule(
            "i.landsat.acca",
            input=self.input_basename,
            output=shadow_output,
            flags="s",
            overwrite=True,
        )
        self.runModule(
            "r.mapcalc",
            expression=f"{shadow_output} = if({shadow_output} > 0, 1, 0)",
            overwrite=True,
        )
        shadow_array = array.array(shadow_output)
        self.assertGreater(
            np.sum(shadow_array),
            0,
            "Cloud shadow detection should identify at least some shadow pixels",
        )
        reference_stats = {
            "sum": 2401,
        }
        self.assertRasterFitsUnivar(
            "cloud_shadow_test", reference_stats, precision=1e-6
        )
        self.runModule(
            "g.remove", type="raster", name=shadow_output, flags="f", quiet=True
        )

    def test_b45ratio_threshold_effect_on_cloud_detection(self):
        """Test the b45ratio parameter to ensure it correctly adjusts cloud detection based on NIR/Red reflectance ratio."""
        temp_basename = "temp_B"
        self.runModule("g.region", n=10, s=0, e=10, w=0, rows=10, cols=10)
        self.runModule(
            "r.mapcalc", expression=f"{temp_basename}2 = 0.4", overwrite=True
        )
        self.runModule(
            "r.mapcalc", expression=f"{temp_basename}3 = 0.5", overwrite=True
        )
        self.runModule(
            "r.mapcalc", expression=f"{temp_basename}61 = 15.0", overwrite=True
        )
        self.runModule(
            "r.mapcalc", expression=f"{temp_basename}5 = 0.3", overwrite=True
        )
        self.runModule(
            "r.mapcalc",
            expression=f"{temp_basename}4 = if(row() < 5, 0.7, 0.4)",
            overwrite=True,
        )
        self.assertModule(
            "i.landsat.acca",
            input=temp_basename,
            output="b45ratio_test_cloud",
            overwrite=True,
        )
        self.assertModule(
            "i.landsat.acca",
            input=temp_basename,
            output="b45ratio_test_cloud_high",
            b45ratio=1.5,
            overwrite=True,
        )
        cloud_default = array.array("b45ratio_test_cloud")
        cloud_high_ratio = array.array("b45ratio_test_cloud_high")
        reference_stats_b45ratio = {
            "sum": 600,
        }
        self.assertRasterFitsUnivar(
            "b45ratio_test_cloud", reference_stats_b45ratio, precision=1e-6
        )
        reference_stats_highb45ratio = {
            "sum": 240,
        }
        self.assertRasterFitsUnivar(
            "b45ratio_test_cloud_high", reference_stats_highb45ratio, precision=1e-6
        )
        self.assertGreater(
            np.sum(cloud_default),
            np.sum(cloud_high_ratio),
            "Higher b45ratio threshold should result in fewer detected cloud pixels",
        )
        self.runModule(
            "g.remove",
            type="raster",
            name=f"{temp_basename}2,{temp_basename}3,{temp_basename}4,{temp_basename}5,{temp_basename}61",
            flags="f",
            quiet=True,
        )
        self.runModule("g.region", n=50, s=0, e=50, w=0, rows=100, cols=100)


if __name__ == "__main__":
    test()
