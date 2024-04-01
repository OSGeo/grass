from grass.gunittest.case import TestCase
import time

class TestReport(TestCase):
    
    @classmethod
    def setUpClass(cls):
        """Use temporary region settings"""
        cls.runModule("g.region", raster="lsat7_2002_10@PERMANENT")
        cls.use_temp_region()
    
    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def test_pca_sample(self):
        """Testing pca sample"""
        self.assertModule("i.pca", input="lsat7_2002_10,lsat7_2002_20,lsat7_2002_30,lsat7_2002_40,lsat7_2002_50,lsat7_2002_70", output="lsat7_2002_pca")
        
        values = "north=228513\nsouth=214975.5\neast=645012\nwest=629992.5\nnsres=28.5\newres=28.5\nrows=475\ncols=527\ncells=250325\ndatatype=CELL\nncats=0\ncomments=\"Eigen values, (vectors), and [percent importance]:PC1   4334.35 ( 0.2824, 0.3342, 0.5092,-0.0087, 0.5264, 0.5217) [83.04%]PC2    588.31 ( 0.2541, 0.1885, 0.2923,-0.7428,-0.5110,-0.0403) [11.27%]PC3    239.22 ( 0.3801, 0.3819, 0.2681, 0.6238,-0.4000,-0.2980) [ 4.58%]PC4     32.85 ( 0.1752,-0.0191,-0.4053, 0.1593,-0.4435, 0.7632) [ 0.63%]PC5     20.73 (-0.6170,-0.2514, 0.6059, 0.1734,-0.3235, 0.2330) [ 0.40%]PC6      4.08 (-0.5475, 0.8021,-0.2282,-0.0607,-0.0208, 0.0252) [ 0.08%]i.pca input=\"lsat7_2002_10,lsat7_2002_20,lsat7_2002_30,lsat7_2002_40\,lsat7_2002_50,lsat7_2002_70\" output=\"lsat7_2002_pca\" rescale=0,255 \\percent=99\""

        self.assertRasterFitsInfo(raster="lsat7_2002_pca.1", reference=values, precision=0)





if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
