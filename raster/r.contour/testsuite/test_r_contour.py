"""
Name:       r.contour test
Purpose:    Tests r.contour module and its options.

Author:     Shubham Sharma, Google Code-in 2018
Copyright:  (C) 2018 by Shubham Sharma and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
            License (>=v2). Read the file COPYING that comes with GRASS
            for details.
"""

from grass.gunittest.case import TestCase
import os

class TestRasterWhat(TestCase):
    input = 'elevation'
    output = 'elevationVector'
    step = 100
    levels = (60, 90, 120, 150)
    minlevel = 1000
    maxlevel = 2000
    cut = 200
    test_ref_str = "cat|level\n1|60\n2|90\n3|120\n4|150\n"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule('g.region', raster=cls.input)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

        cls.runModule('g.remove', type='vector', flags='f', name=cls.output)
        cls.runModule('g.remove', type='vector', flags='f', name=cls.output+"_cut")
        cls.runModule('g.remove', type='vector', flags='f', name=cls.output+"_cut_flag_t")

        if os.path.isfile('testReport'):
            os.remove('testReport')
        if os.path.isfile('testReportCut'):
            os.remove('testReportCut')
        if os.path.isfile('testReportCutFlagT'):
            os.remove('testReportCutFlagT')



    def test_raster_contour(self):
        """Testing r.contour runs successfully with input steps,levels, minlevel, maxlevel"""
        self.assertModule('r.contour', input=self.input, output=self.output, step=self.step, levels=self.levels, minlevel=self.minlevel, maxlevel=self.maxlevel)
        self.assertVectorExists(name=self.output, msg=self.output+" was not created.")

        # Check the attribute values of contours with v.db.select
        self.assertModule('v.db.select', map=self.output, file='testReport')
        self.assertFileExists('testReport', msg='testReport file was not created')
        if os.path.isfile('testReport'):
            file = open("testReport", "r")
            fileData = file.read()
            self.assertMultiLineEqual(fileData, self.test_ref_str)
            file.close()

    def test_raster_contour_cut(self):
        """Testing r.contour runs successfully with input steps,levels, minlevel, maxlevel and cut=100"""
        self.assertModule('r.contour', input=self.input, output=self.output+"_cut", step=self.step, levels=self.levels, minlevel=self.minlevel, maxlevel=self.maxlevel,cut=self.cut)
        self.assertVectorExists(name=self.output+"_cut", msg=self.output+" was not created.")

        # Check the attribute values of contours with v.db.select
        self.assertModule('v.db.select', map=self.output+"_cut", file='testReportCut')
        self.assertFileExists('testReportCut', msg='testReportCut file was not created')
        if os.path.isfile('testReportCut'):
            file = open("testReportCut", "r")
            fileData = file.read()
            self.assertMultiLineEqual(fileData, self.test_ref_str)
            file.close()

    def test_raster_contour_flag_t(self):
        """Testing r.contour runs successfully with input steps,levels, minlevel, maxlevel ,cut=100 and flag t"""
        self.assertModule('r.contour', input=self.input, output=self.output+"_cut_flag_t", flags='t', step=self.step, levels=self.levels, minlevel=self.minlevel, maxlevel=self.maxlevel, cut=self.cut)
        self.assertVectorExists(name=self.output+"_cut_flag_t", msg=self.output+" was not created.")
        # No need to check the attribute values of contours because attribute table was not created

if __name__ == '__main__':
    from grass.gunittest.main import test
    test()
