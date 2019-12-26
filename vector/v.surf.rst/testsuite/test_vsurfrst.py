# -*- coding: utf-8 -*-
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule


class TestVsurfrst(TestCase):

    elevation = 'elevation'
    elevation_attrib = 'elevation_attrib'
    elevation_threads = 'elevation_threads'
    slope = 'slope'
    aspect = 'aspect'
    pcurvature = 'pcurvature'
    tcurvature = 'tcurvature'
    mcurvature = 'mcurvature'
    deviations = 'deviations'
    cvdev = 'cvdev'
    treeseg = 'treeseg'
    overwin = 'overwin'

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule('g.region', vector='elev_lid792_randpts', res=1)
        cls.runModule('v.to.3d', input='elev_lid792_randpts', type='point',
                      output='elev_points3d', column='value', overwrite=True)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        cls.runModule('g.remove', type=['raster', 'vector'],
                      name=['elev_points3d', cls.elevation, cls.elevation_threads,
                            cls.elevation_attrib, cls.slope, cls.aspect, cls.pcurvature,
                            cls.tcurvature, cls.mcurvature, cls.deviations,
                            cls.cvdev, cls.treeseg, cls.overwin], flags='f')

    def setUp(self):
        self.vsurfrst = SimpleModule('v.surf.rst', input='elev_points3d', npmin=100,
                                     elevation=self.elevation, overwrite=True)

    def test_more_threads(self):
        self.assertModule(self.vsurfrst)
        try:
            self.vsurfrst.inputs['nprocs'].value = 4
            self.vsurfrst.outputs.elevation = self.elevation_threads
            self.assertModule(self.vsurfrst)
            self.assertRastersNoDifference(self.elevation, self.elevation_threads, precision=1e-8)
        except KeyError:
            # original version of v.surf.rst without parallel processing
            return

    def test_run_outputs(self):
        self.vsurfrst.outputs.slope = self.slope
        self.vsurfrst.outputs.aspect = self.aspect
        self.vsurfrst.outputs.pcurvature = self.pcurvature
        self.vsurfrst.outputs.tcurvature = self.tcurvature
        self.vsurfrst.outputs.mcurvature = self.mcurvature
        self.vsurfrst.outputs.deviations = self.deviations
        self.vsurfrst.outputs.treeseg = self.treeseg
        self.vsurfrst.outputs.overwin = self.overwin

        self.assertModule(self.vsurfrst)
        self.assertRasterExists(name=self.elevation)
        self.assertRasterExists(name=self.slope)
        self.assertRasterExists(name=self.aspect)
        self.assertRasterExists(name=self.pcurvature)
        self.assertRasterExists(name=self.tcurvature)
        self.assertRasterExists(name=self.mcurvature)
        self.assertVectorExists(name=self.deviations)
        self.assertVectorExists(name=self.treeseg)
        self.assertVectorExists(name=self.overwin)
        values = 'min=103.973861694336\nmax=131.529937744141\nmean=120.774013407641'
        self.assertRasterFitsUnivar(raster=self.elevation, reference=values, precision=1e-8)
        # slope
        values = 'min=0.00417369091883302\nmax=15.4391813278198\nmean=3.32303673469512'
        self.assertRasterFitsUnivar(raster=self.slope, reference=values, precision=1e-8)
        # aspect
        values = 'min=0\nmax=360\nmean=212.026580596575'
        self.assertRasterFitsUnivar(raster=self.aspect, reference=values, precision=1e-8)
        # pcurvature
        values = 'min=-0.0507194809615612\nmax=0.0395903363823891\nmean=0.00013527328666273'
        self.assertRasterFitsUnivar(raster=self.pcurvature, reference=values, precision=1e-8)
        # tcurvature
        values = 'min=-0.0455724261701107\nmax=0.0380486063659191\nmean=-0.000136686790876467'
        self.assertRasterFitsUnivar(raster=self.tcurvature, reference=values, precision=1e-8)
        # mcurvature
        values = 'min=-0.0437114611268044\nmax=0.032054178416729\nmean=-6.78450785489373e-07'
        self.assertRasterFitsUnivar(raster=self.mcurvature, reference=values, precision=1e-8)
        # deviations
        values = 'min=-0.035444\nmax=0.048801\nmean=4.21945e-05'
        self.assertVectorFitsUnivar(map=self.deviations, column='flt1', reference=values, precision=1e-8)
        # treeseg
        topology = dict(primitives=256)
        self.assertVectorFitsTopoInfo(vector=self.treeseg, reference=topology)
        # overwin
        topology = dict(primitives=256)
        self.assertVectorFitsTopoInfo(vector=self.overwin, reference=topology)

        # test 3D versus attribute
        self.vsurfrst.outputs.elevation = self.elevation_attrib
        self.vsurfrst.inputs.column = 'value'
        self.assertModule(self.vsurfrst)
        self.assertRastersNoDifference(self.elevation, self.elevation_attrib, precision=1e-8)

if __name__ == '__main__':
    test()
