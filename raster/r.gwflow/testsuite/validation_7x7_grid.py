"""Test to verify r.gwflow calculation, this calculation is based on
the example at page 133 of the following book:
author = "Kinzelbach, W. and Rausch, R.",
title = "Grundwassermodellierung",
publisher = "Gebr{\"u}der Borntraeger (Berlin, Stuttgart)",
year = "1995"

@author Soeren Gebbert
"""
from grass.gunittest.case import TestCase

class Validation7x7Grid(TestCase):

    @classmethod
    def setUpClass(cls):
        """Use temporary region settings"""
        cls.use_temp_region()
        cls.runModule("g.region", res=100, n=700, s=0, w=0, e=700)

    @classmethod
    def tearDownClass(cls):
        """!Remove the temporary region
        """
        cls.del_temp_region()

    def setUp(self):
        """Create input data for transient groundwater flow computation
        """
        self.runModule("r.mapcalc", expression="phead=50")
        self.runModule("r.mapcalc", expression="status=if(col() == 1 || col() == 7 , 2, 1)")
        self.runModule("r.mapcalc", expression="well=if((row() == 4 && col() == 4), -0.1, 0)")
        self.runModule("r.mapcalc", expression="hydcond=0.0005")
        self.runModule("r.mapcalc", expression="recharge=0")
        self.runModule("r.mapcalc", expression="top_conf=20")
        self.runModule("r.mapcalc", expression="bottom=0")
        self.runModule("r.mapcalc", expression="s=0.0001")
        self.runModule("r.mapcalc", expression="null=0.0")

    def test_transient(self):
        #First compute the groundwater flow after 500 seconds to have initial conditions
        self.assertModule("r.gwflow", flags="f", solver="cholesky", top="top_conf", bottom="bottom", phead="phead",\
         status="status", hc_x="hydcond", hc_y="hydcond", q="well", s="s",\
         recharge="recharge", output="gwresult_conf", dtime=500, type="confined", budget="water_budget",  overwrite=True)

        # loop over the timesteps each 500 seconds
        for i in range(20):
            self.assertModule("r.gwflow",  flags="f", solver="cholesky", top="top_conf", bottom="bottom", phead="gwresult_conf",\
             status="status", hc_x="hydcond", hc_y="hydcond", q="well", s="s",\
             recharge="recharge", output="gwresult_conf", dtime=500, type="confined", budget="water_budget",  overwrite=True)

        # Output of r.univar
        univar_string="""n=49
        null_cells=0
        cells=49
        min=45.1219899394172
        max=50
        range=4.8780100605828
        mean=49.081632669812
        mean_of_abs=49.081632669812
        stddev=0.908558909200636
        variance=0.825479291487849
        coeff_var=1.85111794326975
        sum=2405.00000082079"""
        
        # Output of r.info, only a subset of the output is needed
        info_string="""north=700
        south=0
        east=700
        west=0
        nsres=100
        ewres=100
        rows=7
        cols=7
        cells=49
        datatype=DCELL
        ncats=0
        min=45.1219899394172
        max=50
        map=gwresult_conf"""
        
        self.assertRasterFitsUnivar(raster="gwresult_conf",  reference=univar_string,  precision=3)
        self.assertRasterFitsInfo(raster="gwresult_conf",  reference=info_string,  precision=3)


if __name__ == '__main__':
    from grass.gunittest.main import test
    test()


