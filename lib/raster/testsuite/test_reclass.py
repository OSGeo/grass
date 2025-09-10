from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestRRandomSurface(TestCase):
    data_raster = "data_raster"
    mask_raster = "mask_raster"

    @classmethod
    def setUpClass(cls):
        """Setup input rasters and configure test environment."""
        cls.use_temp_region()
        cls.runModule("g.region", n=20, s=0, e=20, w=0, rows=20, cols=20)
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.data_raster} = row() + col()",
            quiet=True,
        )
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.mask_raster} = if(row() < 10, 1, null())",
            quiet=True,
        )

    @classmethod
    def tearDownClass(cls):
        """Remove generated rasters and reset test environment."""
        cls.runModule(
            "g.remove",
            type="raster",
            name=[cls.mask_raster],
            flags="f",
            quiet=True,
        )
        cls.del_temp_region()

    def test_with_mask(self):
        """Test with a raster mask applied and validate stats."""
        # Ideally with mask manager, but trying to mimic the conditions
        # of a r.random.surface test.
        self.runModule("r.mask", raster=self.mask_raster)
        self.assertModule("r.univar", map=self.data_raster)
        self.runModule("r.mask", flags="r")


if __name__ == "__main__":
    test()
