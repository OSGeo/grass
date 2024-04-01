"""
Name:       i.pca test
Purpose:    Tests i.pca.

Author:     Hamed Elgizery - hamedashraf2004@gmail.com
Copyright:  (C) 2024 by Hamed Elgizery and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
                License (>=v2). Read the file COPYING that comes with GRASS
                for details.
"""

import os

from grass.gunittest.case import TestCase

class TestReport(TestCase):
    
    @classmethod
    def setUpClass(cls):
        """Use temporary region settings"""
        cls.data_path = "data"
        cls.runModule("g.region", raster="lsat7_2002_10@PERMANENT")
        cls.use_temp_region()
    
    @classmethod
    def tearDownClass(cls):
        cls.runModule("g.remove", flags="f", type="raster", name="lsat7_2002_pca.1")
        cls.runModule("g.remove", flags="f", type="raster", name="lsat7_2002_pca.2")
        cls.runModule("g.remove", flags="f", type="raster", name="lsat7_2002_pca.3")
        cls.runModule("g.remove", flags="f", type="raster", name="lsat7_2002_pca.4")
        cls.runModule("g.remove", flags="f", type="raster", name="lsat7_2002_pca.5")
        cls.runModule("g.remove", flags="f", type="raster", name="lsat7_2002_pca.6")
        cls.del_temp_region()

    def test_pca_sample(self):
        """Testing pca sample"""
        self.assertModule("i.pca", input="lsat7_2002_10,lsat7_2002_20,lsat7_2002_30,lsat7_2002_40,lsat7_2002_50,lsat7_2002_70", output="lsat7_2002_pca")
        

        # Asserting the results givien from r.info
        with open(os.path.join(self.data_path, "lsat7_2002_pca_info.out")) as ref_output:
            values = ref_output.read()
            self.assertRasterFitsInfo(raster="lsat7_2002_pca.1", reference=values, precision=0)
            self.assertRasterFitsInfo(raster="lsat7_2002_pca.2", reference=values, precision=0)
            self.assertRasterFitsInfo(raster="lsat7_2002_pca.3", reference=values, precision=0)
            self.assertRasterFitsInfo(raster="lsat7_2002_pca.4", reference=values, precision=0)
            self.assertRasterFitsInfo(raster="lsat7_2002_pca.5", reference=values, precision=0)
            self.assertRasterFitsInfo(raster="lsat7_2002_pca.6", reference=values, precision=0)

        # Asserting the results givien from r.univar
        for i in range(1, 7):
            with open(os.path.join(self.data_path, "lsat7_2002_pca_{}_univar.out".format(i))) as ref_output:
                values = ref_output.read()
                self.assertModuleKeyValue("r.univar", flags="eg", map="lsat7_2002_pca.{}".format(i), reference=values, precision=0, sep="=")





if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
