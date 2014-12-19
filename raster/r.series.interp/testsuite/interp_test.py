"""Test of r.series.interp
@author Soeren Gebbert
"""
from grass.gunittest.case import TestCase

class InterpolationTest(TestCase):

    @classmethod
    def setUpClass(cls):
        """Use temporary region settings"""
        cls.use_temp_region()
        cls.runModule("g.region", res=10, n=80, s=0, w=0, e=120)
        cls.runModule("g.gisenv", set="OVERWRITE=1")

    @classmethod
    def tearDownClass(cls):
        """!Remove the temporary region
        """
        cls.del_temp_region()

    def setUp(self):
        """Generate interpolation data
        """
        self.runModule("r.mapcalc", expression="prec_1 = 100")
        self.runModule("r.mapcalc", expression="prec_5 = 500")
        self.runModule("r.mapcalc", expression="map_10 = 10")
        self.runModule("r.mapcalc", expression="map_20 = 20")
        self.runModule("r.mapcalc", expression="map_30 = 30")
        self.runModule("r.mapcalc", expression="map_40 = 40")
        
    def test_commandline(self):
        self.assertModule("r.series.interp", input="prec_1,prec_5",  datapos=(0.0,1.0),  
            output="prec_2,prec_3,prec_4",  samplingpos=(0.25,0.5,0.75),  method="linear")
        
        self.assertRasterMinMax(map="prec_2",  refmin=200,  refmax=200)
        self.assertRasterMinMax(map="prec_3",  refmin=300,  refmax=300)
        self.assertRasterMinMax(map="prec_4",  refmin=400,  refmax=400)
        
    def test_infile(self):
        self.assertModule("r.series.interp", input="prec_1,prec_5",  datapos=(0.0,1.0),  
            outfile="data/outfile_1.txt",  method="linear")
        
        self.assertRasterMinMax(map="prec_2",  refmin=200,  refmax=200)
        self.assertRasterMinMax(map="prec_3",  refmin=300,  refmax=300)
        self.assertRasterMinMax(map="prec_4",  refmin=400,  refmax=400)

    def test_inoutfiles(self):
        self.assertModule("r.series.interp", infile="data/infile_2.txt",  
            outfile="data/outfile_2.txt",  method="linear")

        self.assertRasterMinMax(map="map_12",  refmin=12,  refmax=12)
        self.assertRasterMinMax(map="map_14",  refmin=14,  refmax=14)
        self.assertRasterMinMax(map="map_16",  refmin=16,  refmax=16)
        self.assertRasterMinMax(map="map_18",  refmin=18,  refmax=18)
        self.assertRasterMinMax(map="map_25",  refmin=25,  refmax=25)
        self.assertRasterMinMax(map="map_35",  refmin=35,  refmax=35)


    def test_module_failure(self):
        """ We need tests to check the failure handling, as outputs, file and 
             sampling points  are not handled by the grass parser"""

        # No outputs
        self.assertModuleFail("r.series.interp", input="prec_1,prec_5",  datapos=(0.0,1.0),  
             samplingpos=(0.25,0.5,0.75),  method="linear")
        # No sampling points
        self.assertModuleFail("r.series.interp", input="prec_1,prec_5",  
            datapos=(0.0,1.0),  output="prec_2,prec_3,prec_4")
        # Output and file at once
        self.assertModuleFail("r.series.interp", input="prec_1,prec_5",  datapos=(0.0,1.0),  
            outfile="outfile_1.txt",  output="prec_2,prec_3,prec_4",  samplingpos=(0.25,0.5,0.75), 
            method="linear")
        # Sampling points and file at once
        self.assertModuleFail("r.series.interp", input="prec_1,prec_5",  datapos=(0.0,1.0),  
            outfile="outfile_1.txt",  samplingpos=(0.25,0.5,0.75), method="linear")
        # Wrong input file
        self.assertModuleFail("r.series.interp", input="prec_1,prec_5",  datapos=(0.0,1.0),  
            outfile="mo_such_file",  method="linear")
        # Wrong input file
        self.assertModuleFail("r.series.interp", input="prec_1,prec_5",  datapos=(0.0,1.0),  
            outfile="outfile_corrupt.txt",  method="linear")

if __name__ == '__main__':
    from grass.gunittest.main import test
    test()


