from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import call_module

SMALL_MAP = """\
north:   15
south:   10
east:    25
west:    20
rows:    5
cols:    5

100.0 150.0 150.0 100.0 100.0
100.0 150.0 150.0 100.0 100.0
100.0 150.0 150.0 150.0 150.0
100.0 150.0 150.0 100.0 100.0
100.0 150.0 150.0 100.0 100.0
"""

class TestSlopeAspect(TestCase):

    def test_limits(self):
        slope = 'limits_slope'
        aspect = 'limits_aspect'
        self.assertModule('r.slope.aspect', elevation='elevation',
                          slope=slope, aspect=aspect)
        self.assertRasterMinMax(map=slope, refmin=0, refmax=90,
                                msg="Slope in degrees must be between 0 and 90")
        self.assertRasterMinMax(map=aspect, refmin=0, refmax=360,
                                msg="Aspect in degrees must be between 0 and 360")

    def test_limits_precent(self):
        slope = 'limits_percent_slope'
        aspect = 'limits_percent_aspect'
        self.assertModule('r.slope.aspect', elevation='elevation',
                          slope=slope, aspect=aspect, format='percent')
        self.assertRasterMinMax(map=slope, refmin=0, refmax=100,
                                msg="Slope in percent must be between 0 and 100")
        self.assertRasterMinMax(map=aspect, refmin=0, refmax=360,
                                msg="Aspect in degrees must be between 0 and 360")


class TestSlopeAspectAgainstReference(TestCase):
    """

    Data created using::

        g.region n=20 s=10 e=25 w=15 res=1
        r.surf.fractal output=fractal_surf
        r.out.ascii input=fractal_surf output=data/fractal_surf.ascii
        gdaldem slope .../fractal_surf.ascii .../gdal_slope.grd -of GSAG
        gdaldem aspect .../fractal_surf.ascii .../gdal_aspect.grd -of GSAG -trigonometric

    GDAL version 1.11.0 was used. Note: GDAL-slope/aspect implementation is originally based on
    GRASS GIS 4.1.
    """

    # precision for comparisons
    precision = 0.0001

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        call_module('g.region', n=20, s=10, e=25, w=15, res=1)
        cls.elevation = 'fractal_surf'
        cls.runModule('r.in.ascii', input='data/fractal_surf.ascii',
                       output=cls.elevation)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        cls.runModule('g.remove', flags='f', type='raster', name=cls.elevation)

    def test_slope(self):
        ref_slope = 'reference_slope'
        slope = 'fractal_slope'

        # TODO: using gdal instead of ascii because of cannot seek error
        self.runModule('r.in.gdal', flags='o',
                       input='data/gdal_slope.grd', output=ref_slope)
        self.assertModule('r.slope.aspect', elevation=self.elevation,
                          slope=slope)
        # check we have expected values
        self.assertRasterMinMax(map=slope, refmin=0, refmax=90,
                                msg="Slope in degrees must be between 0 and 90")
        # check against reference data
        self.assertRastersNoDifference(actual=slope, reference=ref_slope,
                                       precision=self.precision)

    def test_aspect(self):
        ref_aspect = 'reference_aspect'
        aspect = 'fractal_aspect'
        # TODO: using gdal instead of ascii because of cannot seek error
        self.runModule('r.in.gdal', flags='o',
                       input='data/gdal_aspect.grd', output=ref_aspect)
        self.assertModule('r.slope.aspect', elevation=self.elevation,
                          aspect=aspect)
        # check we have expected values
        self.assertRasterMinMax(map=aspect, refmin=0, refmax=360,
                                msg="Aspect in degrees must be between 0 and 360")
        # check against reference data
        self.assertRastersNoDifference(actual=aspect, reference=ref_aspect,
                                       precision=self.precision)


class TestSlopeAspectAgainstItself(TestCase):

    precision = 0.0000001

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        call_module('g.region', raster='elevation')

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def test_slope_aspect_together(self):
        """Slope and aspect computed separately and together should be the same
        """
        elevation = 'elevation'
        t_aspect = 'sa_together_aspect'
        t_slope = 'sa_together_slope'
        s_aspect = 'sa_separately_aspect'
        s_slope = 'sa_separately_slope'
        self.assertModule('r.slope.aspect', elevation=elevation,
                          aspect=s_aspect)
        self.assertModule('r.slope.aspect', elevation=elevation,
                          slope=s_slope)
        self.assertModule('r.slope.aspect', elevation=elevation,
                          slope=t_slope, aspect=t_aspect)
        self.assertRastersNoDifference(actual=t_aspect, reference=s_aspect,
                                       precision=self.precision)
        self.assertRastersNoDifference(actual=t_slope, reference=s_slope,
                                       precision=self.precision)


# TODO: implement this class
class TestExtremes(TestCase):

    def setUp(self):
        self.use_temp_region()

    def tearDown(self):
        self.del_temp_region()

    def test_small(self):
        elevation = 'small_elevation'
        slope = 'small_slope'
        aspect = 'small_aspect'
        self.runModule('r.in.ascii', input='-', output=elevation,
                       stdin_=SMALL_MAP)
        call_module('g.region', raster=elevation)
        self.assertModule('r.slope.aspect', elevation=elevation,
                          slope=slope, aspect=aspect)
        self.assertRasterMinMax(map=slope, refmin=0, refmax=90,
                                msg="Slope in degrees must be between 0 and 90")
        self.assertRasterMinMax(map=aspect, refmin=0, refmax=360,
                                msg="Aspect in degrees must be between 0 and 360")


if __name__ == '__main__':
    test()
