# -*- coding: utf-8 -*-
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule


class TestRSunMode2(TestCase):
    elevation = 'elevation'
    elevation_attrib = 'elevation_attrib'
    elevation_threads = 'elevation_threads'
    slope = 'rsun_slope'
    aspect = 'rsun_aspect'
    beam_rad = 'beam_rad'
    glob_rad = 'glob_rad'
    insol_time = 'insol_time'
    glob_rad_threads = 'glob_rad_threads'

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule('g.region', n=223500, s=220000, e=640000, w=635000, res=10)
        cls.runModule('r.slope.aspect', elevation=cls.elevation, slope=cls.slope, aspect=cls.aspect)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        cls.runModule('g.remove', type=['raster'],
                      name=[cls.slope, cls.aspect, cls.insol_time, cls.beam_rad, cls.glob_rad, cls.glob_rad_threads], flags='f')

    def setUp(self):
        self.rsun = SimpleModule('r.sun', elevation=self.elevation, slope=self.slope, aspect=self.aspect,
                                 day=172, beam_rad=self.beam_rad, glob_rad=self.glob_rad, insol_time=self.insol_time,
                                 overwrite=True)

    def test_more_threads(self):
        self.assertModule(self.rsun)
        try:
            self.rsun.inputs['nprocs'].value = 4
            self.rsun.outputs.glob_rad = self.glob_rad_threads
            self.assertModule(self.rsun)
            self.assertRastersNoDifference(self.glob_rad, self.glob_rad_threads, precision=1e-8)
        except KeyError:
            # original version of r.sun without parallel processing
            return

    def test_run_outputs(self):
        self.assertModule(self.rsun)
        self.assertRasterExists(name=self.beam_rad)
        self.assertRasterExists(name=self.glob_rad)
        self.assertRasterExists(name=self.insol_time)
        # beam_rad
        values = 'min=6561.3505859375\nmax=7680.93505859375\nmean=7638.91454059886'
        self.assertRasterFitsUnivar(raster=self.beam_rad, reference=values, precision=1e-8)
        # glob_rad
        values = 'min=7829.4921875\nmax=8889.4765625\nmean=8851.4872842213'
        self.assertRasterFitsUnivar(raster=self.glob_rad, reference=values, precision=1e-8)
        # insol_time
        values = 'min=11.5\nmax=14\nmean=13.8521038175691'
        self.assertRasterFitsUnivar(raster=self.insol_time, reference=values, precision=1e-8)


class TestRSunMode1(TestCase):
    elevation = 'elevation'
    elevation_attrib = 'elevation_attrib'
    elevation_threads = 'elevation_threads'
    slope = 'rsun_slope'
    aspect = 'rsun_aspect'
    beam_rad = 'beam_rad'
    glob_rad = 'glob_rad'
    incidout = 'incidout'
    glob_rad_threads = 'glob_rad_threads'

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule('g.region', n=223500, s=220000, e=640000, w=635000, res=10)
        cls.runModule('r.slope.aspect', elevation=cls.elevation, slope=cls.slope, aspect=cls.aspect)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        cls.runModule('g.remove', type=['raster'],
                      name=[cls.slope, cls.aspect, cls.incidout, cls.beam_rad, cls.glob_rad, cls.glob_rad_threads], flags='f')

    def setUp(self):
        self.rsun = SimpleModule('r.sun', elevation=self.elevation, slope=self.slope, aspect=self.aspect,
                                 day=172, time=18, beam_rad=self.beam_rad, glob_rad=self.glob_rad, incidout=self.incidout,
                                 overwrite=True)

    def test_more_threads(self):
        self.assertModule(self.rsun)
        try:
            self.rsun.inputs['nprocs'].value = 4
            self.rsun.outputs.glob_rad = self.glob_rad_threads
            self.assertModule(self.rsun)
            self.assertRastersNoDifference(self.glob_rad, self.glob_rad_threads, precision=1e-8)
        except KeyError:
            # original version of r.sun without parallel processing
            return

    def test_run_outputs(self):
        self.assertModule(self.rsun)
        self.assertRasterExists(name=self.beam_rad)
        self.assertRasterExists(name=self.glob_rad)
        self.assertRasterExists(name=self.incidout)
        # beam_rad
        values = 'min=0\nmax=355.780944824219\nmean=124.615522080923'
        self.assertRasterFitsUnivar(raster=self.beam_rad, reference=values, precision=1e-8)
        # glob_rad
        values = 'min=32.1095008850098\nmax=451.527923583984\nmean=178.057346498427'
        self.assertRasterFitsUnivar(raster=self.glob_rad, reference=values, precision=1e-8)
        # incidout
        values = 'min=0.0100772567093372\nmax=40.5435371398926\nmean=13.2855086254291'
        self.assertRasterFitsUnivar(raster=self.incidout, reference=values, precision=1e-8)

if __name__ == '__main__':
    test()
