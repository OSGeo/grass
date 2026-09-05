"""Test check stability of results from r.terraflow
@author Stefan Blumentrath, NINA
"""

import os
import tempfile
from pathlib import Path
from grass.gunittest.case import TestCase


class TestTerraflow(TestCase):
    elevation = "elevation"
    testdir = os.path.join(tempfile.gettempdir(), "terraflow_test")
    teststats = os.path.join(tempfile.gettempdir(), "terraflow_test_stats.txt")

    @classmethod
    def setUpClass(cls):
        """Use temporary region settings"""
        cls.use_temp_region()
        cls.runModule("g.region", flags="p", raster=cls.elevation)

    @classmethod
    def tearDownClass(cls):
        """!Remove the temporary region"""
        cls.del_temp_region()

    def setUp(self):
        """Create input data for steady state groundwater flow computation"""
        Path(self.testdir).mkdir(exist_ok=True)

    def test_univar_mfd(self):
        # compute a steady state groundwater flow
        self.assertModule(
            "r.terraflow",
            overwrite=True,
            verbose=True,
            elevation=self.elevation,
            filled="terra_flooded",
            direction="terra_flowdir",
            swatershed="terra_sink",
            accumulation="terra_flowaccum",
            tci="terra_tci",
            directory=self.testdir,
            stats=self.teststats,
        )

        # Output of r.univar -g
        terra_flooded_univar = """n=2025000
null_cells=0
cells=2025000
min=55.5787925720215
max=156.329864501953
range=100.751071929932
mean=110.466900511132
mean_of_abs=110.466900511132
stddev=20.2568412316924
variance=410.339616685993
coeff_var=18.3374758755462
sum=223695473.535042"""

        terra_flowdir_univar = """n=2025000
null_cells=0
cells=2025000
min=0
max=255
range=255
mean=114.239481481481
mean_of_abs=114.239481481481
stddev=84.9304048144913
variance=7213.17366195336
coeff_var=74.344179186649
sum=231334950"""

        terra_sink_univar = """n=2025000
null_cells=0
cells=2025000
min=0
max=7945
range=7945
mean=3716.98878864198
mean_of_abs=3716.98878864198
stddev=2352.78190064133
variance=5535582.67198542
coeff_var=63.2980628790363
sum=7526902297"""

        terra_flowaccum_univar = """n=2025000
null_cells=0
cells=2025000
min=1
max=638570.4375
range=638569.4375
mean=644.701550164795
mean_of_abs=644.701550164795
stddev=10616.1468932394
variance=112702574.858836
coeff_var=1646.67618536449
sum=1305520639.08371"""

        terra_tci_univar = """n=2025000
null_cells=0
cells=2025000
min=1.07463788986206
max=16.7091903686523
range=15.6345524787903
mean=4.11934358476421
mean_of_abs=4.11934358476421
stddev=1.97140337926634
variance=3.88643128378274
coeff_var=47.8572213922083
sum=8341670.75914752"""

        self.assertRasterFitsUnivar(
            raster="terra_flooded", reference=terra_flooded_univar, precision=3
        )
        self.assertRasterFitsUnivar(
            raster="terra_flowdir", reference=terra_flowdir_univar, precision=3
        )
        self.assertRasterFitsUnivar(
            raster="terra_sink", reference=terra_sink_univar, precision=3
        )
        self.assertRasterFitsUnivar(
            raster="terra_flowaccum", reference=terra_flowaccum_univar, precision=3
        )
        self.assertRasterFitsUnivar(
            raster="terra_tci", reference=terra_tci_univar, precision=3
        )


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
