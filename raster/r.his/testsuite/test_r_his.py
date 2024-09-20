from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestRHis(TestCase):
    hue = "elevation"
    intensity = "elevation_shaded_relief_50"
    saturation = "elevation_g"
    red = "shadedmap_r"
    green = "shadedmap_g"
    blue = "shadedmap_b"
    bgcolor = ["none", "0:90:255"]

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule("r.relief", input="elevation", output="elevation_shaded_relief")
        cls.runModule(
            "r.rgb",
            input="elevation",
            red="elevation_r",
            green="elevation_g",
            blue="elevation_b",
        )
        cls.runModule(
            "r.mapcalc", expression=f"{cls.intensity} = #elevation_shaded_relief * 1.5"
        )
        cls.runModule("r.colors", map=cls.intensity, color="grey255")

    @classmethod
    def tearDownClass(cls):
        cls.runModule("g.remove", type="raster", flags="f", pattern="elevation_*")
        cls.del_temp_region()

    def tearDown(self):
        """Remove d.his generated rasters after each test method"""
        self.runModule("g.remove", type="raster", flags="f", pattern="shadedmap_*")

    def test_d_his_with_bgcolor_none(self):
        """Test r.his with bgcolor 'none'"""
        self.assertModule(
            "r.his",
            hue=self.hue,
            intensity=self.intensity,
            saturation=self.saturation,
            red=self.red,
            green=self.green,
            blue=self.blue,
            bgcolor=self.bgcolor[0],
        )

    def test_d_his_with_bgcolor_rgb(self):
        """Test r.his with bgcolor '0:90:255'"""
        self.assertModule(
            "r.his",
            hue=self.hue,
            intensity=self.intensity,
            saturation=self.saturation,
            red=self.red,
            green=self.green,
            blue=self.blue,
            bgcolor=self.bgcolor[1],
        )


if __import__("__main__"):
    test()
