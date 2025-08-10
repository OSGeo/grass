from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestRHis(TestCase):
    hue = "elevation"
    intensity = "elevation_shaded_50"
    saturation = "elevation"
    red = "shadedmap_r"
    green = "shadedmap_g"
    blue = "shadedmap_b"
    bgcolor = "none"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule("g.region", raster="elevation")
        cls.elev_shade = "elevation_shaded_relief"
        cls.runModule("r.relief", input="elevation", output=cls.elev_shade)
        cls.runModule(
            "r.mapcalc", expression=f"{cls.intensity} = {cls.elev_shade} * 1.5"
        )
        cls.runModule("r.colors", map=cls.intensity, color="grey255")

    @classmethod
    def tearDownClass(cls):
        cls.runModule(
            "g.remove", type="raster", flags="f", name=[cls.intensity, cls.elev_shade]
        )
        cls.del_temp_region()

    def tearDown(self):
        """Remove d.his generated rasters after each test method"""
        self.runModule("g.remove", type="raster", flags="f", pattern="shadedmap_*")

    def test_bgcolor_none(self):
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

        red_value = "null_cells=5696\nmin=3\nmax=255\nmean=156.41168\nstddev=34.434612"
        blue_value = "null_cells=5696\nmin=0\nmax=127\nmean=36.05560\nstddev=37.61216"
        green_value = "null_cells=5696\nmin=1\nmax=255\nmean=129.62880\nstddev=34.48307"

        self.assertRasterFitsUnivar(
            raster=self.red, reference=red_value, precision=1e-5
        )
        self.assertRasterFitsUnivar(
            raster=self.blue, reference=blue_value, precision=1e-5
        )
        self.assertRasterFitsUnivar(
            raster=self.green, reference=green_value, precision=1e-5
        )

    def test_with_bgcolor_rgb(self):
        """Test r.his with bgcolor '0:0:0'"""
        self.runModule(
            "r.his",
            hue=self.hue,
            intensity=self.intensity,
            saturation=self.saturation,
            red=self.red,
            green=self.green,
            blue=self.blue,
            bgcolor="0:0:0",
        )

        red_value = "null_cells=0\nmin=0\nmax=255\nmean=155.97172\nstddev=35.36988"
        blue_value = "null_cells=0\nmin=0\nmax=127\nmean=35.95417\nstddev=37.60774"
        green_value = "null_cells=0\nmin=0\nmax=255\nmean=129.26418\nstddev=35.11225"
        self.assertRasterFitsUnivar(
            raster=self.red, reference=red_value, precision=1e-5
        )
        self.assertRasterFitsUnivar(
            raster=self.blue, reference=blue_value, precision=1e-5
        )
        self.assertRasterFitsUnivar(
            raster=self.green, reference=green_value, precision=1e-5
        )


if __name__ == "__main__":
    test()
