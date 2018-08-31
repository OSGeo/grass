from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import call_module


class TestViewshed(TestCase):

    viewshed = 'test_viewshed_from_elevation'

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule('g.region', raster='elevation')

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def tearDown(cls):
        """Remove viewshed map after each test method"""
        # TODO: eventually, removing maps should be handled through testing framework fucntions
        cls.runModule('g.remove', flags='f', type='raster',
                      name=cls.viewshed)

    def test_limits(self):
        """Test if results is in expected limits"""
        obs_elev = '1.72'
        self.assertModule('r.viewshed', input='elevation',
            coordinates=(634720,216180), output=self.viewshed,
            observer_elevation=obs_elev)
        self.assertRasterMinMax(map=self.viewshed, refmin=0, refmax=180,
                                msg="Viewing angle above the ground must be between 0 and 180 deg")

    def test_limits_flags(self):
        obs_elev = '1.72'
        # test e flag
        self.assertModule('r.viewshed', input='elevation', flags='e',
                          coordinates=(634720, 216180), output=self.viewshed,
                          observer_elevation=obs_elev)
        minmax = 'null_cells=1963758\nmin=-24.98421\nmax=43.15356'
        self.assertRasterFitsUnivar(raster=self.viewshed, reference=minmax, precision=1e-5)
        # test b flag (#1788)
        self.assertModule('r.viewshed', input='elevation', flags='b',
                          coordinates=(634720, 216180), output=self.viewshed,
                          observer_elevation=obs_elev, overwrite=True)
        minmax = 'min=0\nmax=1\ndatatype=CELL'
        self.assertRasterFitsInfo(raster=self.viewshed, reference=minmax,
                                  msg="Values of binary output must be 0 or 1")

    def test_limits_extreme_value(self):
        """Test extremely high observer elevation

        Unfortunatelly, this takes very long time to compute
        (something like ten times more).
        """
        obs_elev = '500000'
        self.assertModule('r.viewshed', input='elevation',
            coordinates=(634720,216180), output=self.viewshed,
            observer_elevation=obs_elev)
        self.assertRasterMinMax(map=self.viewshed, refmin=0, refmax=180,
            msg="Viewing angle above the ground must be between 0 and 180 deg")


class TestViewshedAgainstReference(TestCase):
    """

    Data created using NC data set::

        g.region n=216990 s=215520 w=633730 e=635950 res=10 -p
        r.out.ascii elevation out=data/elevation.ascii
        r.viewshed input=elevation output=lake_viewshed coordinates=634720,216180 observer_elevation=1.72
        r.out.ascii lake_viewshed out=data/lake_viewshed.ascii

    """

    # precision for comparisons
    precision = 0.0001

    # list to hold rasters to be removed
    # here we accumulate all rasters created during tests
    # then we remove them at the end in class tear down
    to_remove = []

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule('g.region', n=216990, s=215520, e=635950, w=633730, res=10)
        cls.elevation = 'ref_elevation'
        cls.runModule('r.in.ascii', input='data/elevation.ascii',
                       output=cls.elevation)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        cls.runModule('g.remove', flags='f', type='raster',
                      name=cls.elevation)
        if cls.to_remove:
            cls.runModule('g.remove', flags='f', type='raster',
                          name=','.join(cls.to_remove))

    def test_viewshed(self):
        ref_viewshed = 'reference_viewshed'
        viewshed = 'actual_viewshed'
        obs_elev = '1.72'

        self.runModule('r.in.ascii',
                       input='data/lake_viewshed.ascii', output=ref_viewshed)
        self.to_remove.append(ref_viewshed)

        self.assertModule('r.viewshed', input=self.elevation,
                          coordinates=(634720,216180), output=viewshed, observer_elevation=obs_elev)
        self.to_remove.append(viewshed)

        # check we have expected values
        self.assertRasterMinMax(map=viewshed, refmin=0, refmax=180,
                                msg="Viewshed in degrees must be between 0 and 180")
        # check against reference data
        self.assertRastersNoDifference(actual=viewshed, reference=ref_viewshed,
                                       precision=self.precision)
        # TODO: add self.assertRasterFitsUnivar()


if __name__ == '__main__':
    test()
