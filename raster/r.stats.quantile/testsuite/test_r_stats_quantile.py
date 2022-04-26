from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import call_module

# TODO: add verification of r.stats.quantile -p output


class TestStatsQuantile(TestCase):

    base = "zipcodes"
    cover = "elevation"

    def setUp(self):
        self.use_temp_region()
        call_module("g.region", raster="elevation")

    def tearDown(self):
        self.del_temp_region()
        call_module(
            "g.remove",
            flags="f",
            type_="raster",
            name=[self.base, self.cover],
        )

    def test_limits(self):
        self.assertModule(
            "r.stats.quantile",
            base=self.base,
            cover=self.cover,
            quantiles=3,
            flags="p",
        )


if __name__ == "__main__":
    test()
