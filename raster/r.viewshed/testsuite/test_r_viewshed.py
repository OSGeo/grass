import grass.gunittest
from grass.gunittest.gmodules import call_module


class TestViewshed(grass.gunittest.TestCase):

    viewshed = 'test_viewshed_from_elevation'

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule('g.region', rast='elevation')

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def tearDown(cls):
        """Remove viewshed map after each test method"""
        # TODO: eventually, removing maps should be handled through testing framework fucntions
        cls.runModule('g.remove', flags='f', type='rast',
                      pattern=cls.viewshed)

    def test_limits(self):
        """Test if results is in expected limits"""
        obs_elev = '1.72'
        self.assertModule('r.viewshed', input='elevation',
            coordinates=(634720,216180), output=self.viewshed,
            obs_elev=obs_elev)
        self.assertRasterMinMax(map=self.viewshed, refmin=0, refmax=180,
            msg="Viewing angle above the ground must be between 0 and 180 deg")

    def test_limits_extreme_value(self):
        """Test extremely high observer elevation

        Unfortunatelly, this takes very long time to compute
        (something like ten times more).
        """
        obs_elev = '500000'
        self.assertModule('r.viewshed', input='elevation',
            coordinates=(634720,216180), output=self.viewshed,
            obs_elev=obs_elev)
        self.assertRasterMinMax(map=self.viewshed, refmin=0, refmax=180,
            msg="Viewing angle above the ground must be between 0 and 180 deg")


class TestViewshedAgainstReference(grass.gunittest.TestCase):
    """

    Data created using NC data set::

        g.region n=216990 s=215520 w=633730 e=635950 res=10 -p
        r.out.ascii elevation out=data/elevation.ascii
        r.viewshed input=elevation output=lake_viewshed coordinates=634720,216180 obs_elev=1.72
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
        cls.runModule('g.remove', flags='f', type='rast',
                      pattern=cls.elevation)
        if cls.to_remove:
            cls.runModule('g.remove', flags='f', type='rast',
                          pattern=','.join(cls.to_remove))

    def test_viewshed(self):
        ref_viewshed = 'reference_viewshed'
        viewshed = 'actual_viewshed'
        obs_elev = '1.72'

        self.runModule('r.in.ascii',
                       input='data/lake_viewshed.ascii', output=ref_viewshed)
        self.to_remove.append(ref_viewshed)

        self.assertModule('r.viewshed', input=self.elevation,
                          coordinates=(634720,216180), output=viewshed, obs_elev=obs_elev)
        self.to_remove.append(viewshed)

        # check we have expected values
        self.assertRasterMinMax(map=viewshed, refmin=0, refmax=180,
                                msg="Viewshed in degrees must be between 0 and 180")
        # check against reference data
        self.assertRastersNoDifference(actual=viewshed, reference=ref_viewshed,
                                       precision=self.precision)
        # TODO: add self.assertRasterFitsUnivar()


if __name__ == '__main__':
    grass.gunittest.test()
