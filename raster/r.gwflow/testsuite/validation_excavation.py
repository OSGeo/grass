"""Test to verify r.gwflow calculation, this calculation is based on
the example at page 167 of the following book:
author = "Kinzelbach, W. and Rausch, R.",
title = "Grundwassermodellierung",
publisher = "Gebr{\"u}der Borntraeger (Berlin, Stuttgart)",
year = "1995"

@author Soeren Gebbert
"""

import grass.script as grass
import grass.pygrass.modules as pymod
from gunittest.case import TestCase

# Output of r.univar -g
UNIVAR_OUTPUT="""n=760
null_cells=0
cells=760
min=3
max=5.39762629189687
range=2.39762629189687
mean=5.02846950820457
mean_of_abs=5.02846950820457
stddev=0.333565013446849
variance=0.111265618195797
coeff_var=6.63352960384062
sum=3821.63682623547
"""

# Output of r.info -gre, only a subset of the output is needed
INFO_OUTPUT="""north=950
south=0
east=2000
west=0
nsres=50
ewres=50
rows=19
cols=40
cells=760
datatype=DCELL
ncats=0
min=3
max=5.3976262918968
map=gwresult
"""

class ValidationExcavation(TestCase):

    @classmethod
    def setUpClass(cls):
        """!Initiate the temporal GIS and set the region
        """
        # Use always the current mapset as temporal database
        grass.use_temp_region()
        grass.run_command("g.region", flags="p",  res=50, n=950, s=0, w=0, e=2000)

    def setUp(self):
        """Create input data for steady state groundwater flow computation
        """

        self.assertModule("r.mapcalc", expression="phead= if(row() == 19, 5, 3)")
        self.assertModule("r.mapcalc", expression="status=if((col() == 1 && row() == 13) ||\
                                      (col() == 1 && row() == 14) ||\
                                      (col() == 2 && row() == 13) ||\
                                      (col() == 2 && row() == 14) ||\
                                      (row() == 19), 2, 1)")

        self.assertModule("r.mapcalc", expression="hydcond=0.001")
        self.assertModule("r.mapcalc", expression="recharge=0.000000006")
        self.assertModule("r.mapcalc", expression="top=20")
        self.assertModule("r.mapcalc", expression="bottom=0")
        self.assertModule("r.mapcalc", expression="poros=0.1")
        self.assertModule("r.mapcalc", expression="null=0.0")

    def test_steady_state(self):
        #compute a steady state groundwater flow
        grass.run_command("r.gwflow", flags="f", solver="cholesky", top="top", bottom="bottom", phead="phead", \
            status="status", hc_x="hydcond", hc_y="hydcond", s="poros", \
            recharge="recharge", output="gwresult", dt=864000000000, type="unconfined", budget="water_budget")
        
        global UNIVAR_OUTPUT
        self.assertRasterFitsUnivar(raster="gwresult",  reference=UNIVAR_OUTPUT,  precision=3)
        global INFO_OUTPUT
        self.assertRasterFitsInfo(raster="gwresult",  reference=INFO_OUTPUT,  precision=3)

    @classmethod
    def tearDownClass(cls):
        """!Remove the temporary region
        """
        grass.del_temp_region()

if __name__ == '__main__':
    from gunittest.main import test
    test()


