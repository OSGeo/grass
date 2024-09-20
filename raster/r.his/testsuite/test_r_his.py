from grass.gunittest.case import TestCase
from grass.gunittest.main import test

# r.univar value for elevation shaded relief
value = "null_cells=5696"


class TestRHisDefault(TestCase):
    hue = "elevation"
    intensity = "elevation_shaded_50"
    saturation = "elevation"
    red = "shadedmap_r"
    green = "shadedmap_g"
    blue = "shadedmap_b"
    bgcolor = "none"

    @classmethod
    def setUpClass(cls):
        cls.elev_shade = "elevation_shaded_relief"
        cls.use_temp_region()
        cls.runModule("r.relief", input="elevation", output=cls.elev_shade)
        cls.runModule(
            "r.mapcalc", expression=f"{cls.intensity} = {cls.elev_shade} * 1.5"
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

        self.runModule(
            "r.his",
            hue=self.hue,
            intensity=self.intensity,
            saturation=self.saturation,
            red=self.red,
            green=self.green,
            blue=self.blue,
            bgcolor=self.bgcolor,
        )

        self.assertRasterFitsUnivar(raster=self.red, reference=value)
        self.assertRasterFitsUnivar(raster=self.green, reference=value)
        self.assertRasterFitsUnivar(raster=self.blue, reference=value)

    def test_d_his_with_bgcolor_rgb(self):
        """Test r.his with bgcolor '0:0:0'"""
        no_null = "null_cells=0"
        self.runModule(
            "r.his",
            hue=self.hue,
            intensity=self.intensity,
            saturation=self.saturation,
            red=self.red,
            green=self.green,
            blue=self.blue,
            bgcolor="0:0:0:255",
        )

        # Check elevation_shaded_50 has null values
        self.assertRasterFitsUnivar(raster=self.intensity, reference=value)
        # Check shadedmap_r, shadedmap_g, shadedmap_b have no null values
        self.assertRasterFitsUnivar(raster=self.red, reference=no_null)
        self.assertRasterFitsUnivar(raster=self.green, reference=no_null)
        self.assertRasterFitsUnivar(raster=self.blue, reference=no_null)


if __import__("__main__"):
    test()
