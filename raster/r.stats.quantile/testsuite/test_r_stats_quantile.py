from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import call_module
from grass.gunittest.gmodules import SimpleModule

reference_str = """\
27511:0:33.333333:134.717331
27511:1:66.666667:143.985499
27513:0:33.333333:140.669418
27513:1:66.666667:146.279022
27518:0:33.333333:115.140050
27518:1:66.666667:129.893631
27529:0:33.333333:96.325943
27529:1:66.666667:103.571231
27539:0:33.333333:123.216911
27539:1:66.666667:133.431442
27601:0:33.333333:89.850876
27601:1:66.666667:98.379255
27603:0:33.333333:91.602473
27603:1:66.666667:103.511017
27604:0:33.333333:75.695976
27604:1:66.666667:87.048233
27605:0:33.333333:102.274857
27605:1:66.666667:112.512177
27606:0:33.333333:108.092461
27606:1:66.666667:124.441594
27607:0:33.333333:124.740369
27607:1:66.666667:135.907516
27608:0:33.333333:91.426587
27608:1:66.666667:104.532201
27610:0:33.333333:81.664024
27610:1:66.666667:92.704079
"""


class TestStatsQuantile(TestCase):
<<<<<<< HEAD
=======

>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
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

    def test_quantiles(self):
        self.assertModule(
            "r.stats.quantile",
            base=self.base,
            cover=self.cover,
            quantiles=3,
            flags="p",
        )

    def test_flagp(self):
        """Testing flag p with map zipcodes and elevation"""
        module = SimpleModule(
            "r.stats.quantile", base=self.base, cover=self.cover, quantiles=3, flags="p"
        )
        module.run()
        self.assertLooksLike(actual=str(module.outputs.stdout), reference=reference_str)


if __name__ == "__main__":
    test()
