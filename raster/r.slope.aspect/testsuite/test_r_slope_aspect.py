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

    slope = "limits_slope"
    aspect = "limits_aspect"
    slope_threaded = "limits_slope_threaded"
    aspect_threaded = "limits_aspect_threaded"

    def setUp(self):
        self.use_temp_region()
        call_module("g.region", raster="elevation")

    def tearDown(self):
        self.del_temp_region()
        call_module(
            "g.remove",
            flags="f",
            type_="raster",
            name=[self.slope, self.aspect, self.aspect_threaded, self.slope_threaded],
        )

    def test_limits(self):
        self.assertModule(
            "r.slope.aspect",
            elevation="elevation",
            slope=self.slope,
            aspect=self.aspect,
        )
        self.assertModule(
            "r.slope.aspect",
            elevation="elevation",
            slope=self.slope_threaded,
            aspect=self.aspect_threaded,
            nprocs=8,
        )
        self.assertRasterMinMax(
            map=self.slope,
            refmin=0,
            refmax=90,
            msg="Slope in degrees must be between 0 and 90",
        )
        self.assertRasterMinMax(
            map=self.slope_threaded,
            refmin=0,
            refmax=90,
            msg="Slope in degrees must be between 0 and 90",
        )
        self.assertRasterMinMax(
            map=self.aspect,
            refmin=0,
            refmax=360,
            msg="Aspect in degrees must be between 0 and 360",
        )
        self.assertRasterMinMax(
            map=self.aspect_threaded,
            refmin=0,
            refmax=360,
            msg="Aspect in degrees must be between 0 and 360",
        )

    def test_limits_percent(self):
        """Assumes NC elevation and allows slope up to 100% (45deg)"""
        self.assertModule(
            "r.slope.aspect",
            elevation="elevation",
            slope=self.slope,
            aspect=self.aspect,
            format="percent",
        )
        self.assertModule(
            "r.slope.aspect",
            elevation="elevation",
            slope=self.slope_threaded,
            aspect=self.aspect_threaded,
            format="percent",
            nprocs=8,
        )
        self.assertRasterMinMax(
            map=self.slope,
            refmin=0,
            refmax=100,
            msg="Slope in percent must be between 0 and 100",
        )
        self.assertRasterMinMax(
            map=self.slope_threaded,
            refmin=0,
            refmax=100,
            msg="Slope in percent must be between 0 and 100",
        )
        self.assertRasterMinMax(
            map=self.aspect,
            refmin=0,
            refmax=360,
            msg="Aspect in degrees must be between 0 and 360",
        )
        self.assertRasterMinMax(
            map=self.aspect_threaded,
            refmin=0,
            refmax=360,
            msg="Aspect in degrees must be between 0 and 360",
        )


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
    ref_aspect = "reference_aspect"
    aspect = "fractal_aspect"
    aspect_threaded = "fractal_aspect_threaded"
    ref_slope = "reference_slope"
    slope = "fractal_slope"
    slope_threaded = "fractal_slope_threaded"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        call_module("g.region", n=20, s=10, e=25, w=15, res=1)
        cls.elevation = "fractal_surf"
        cls.runModule(
            "r.in.ascii", input="data/fractal_surf.ascii", output=cls.elevation
        )

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        cls.runModule(
            "g.remove",
            flags="f",
            type="raster",
            name=[
                cls.elevation,
                cls.slope,
                cls.slope_threaded,
                cls.aspect,
                cls.aspect_threaded,
                cls.ref_aspect,
                cls.ref_slope,
            ],
        )

    def test_slope(self):
        # TODO: using gdal instead of ascii because of cannot seek error
        self.runModule(
            "r.in.gdal", flags="o", input="data/gdal_slope.grd", output=self.ref_slope
        )
        self.assertModule("r.slope.aspect", elevation=self.elevation, slope=self.slope)
        self.assertModule(
            "r.slope.aspect",
            elevation=self.elevation,
            slope=self.slope_threaded,
            nprocs=8,
        )
        # check we have expected values
        self.assertRasterMinMax(
            map=self.slope,
            refmin=0,
            refmax=90,
            msg="Slope in degrees must be between 0 and 90",
        )
        self.assertRasterMinMax(
            map=self.slope_threaded,
            refmin=0,
            refmax=90,
            msg="Slope in degrees must be between 0 and 90",
        )
        # check against reference data
        self.assertRastersNoDifference(
            actual=self.slope, reference=self.ref_slope, precision=self.precision
        )
        self.assertRastersNoDifference(
            actual=self.slope_threaded,
            reference=self.ref_slope,
            precision=self.precision,
        )

    def test_aspect(self):
        # TODO: using gdal instead of ascii because of cannot seek error
        self.runModule(
            "r.in.gdal", flags="o", input="data/gdal_aspect.grd", output=self.ref_aspect
        )
        self.assertModule(
            "r.slope.aspect", elevation=self.elevation, aspect=self.aspect
        )
        self.assertModule(
            "r.slope.aspect",
            elevation=self.elevation,
            aspect=self.aspect_threaded,
            nprocs=8,
        )
        # check we have expected values
        self.assertRasterMinMax(
            map=self.aspect,
            refmin=0,
            refmax=360,
            msg="Aspect in degrees must be between 0 and 360",
        )
        self.assertRasterMinMax(
            map=self.aspect_threaded,
            refmin=0,
            refmax=360,
            msg="Aspect in degrees must be between 0 and 360",
        )
        # check against reference data
        self.assertRastersNoDifference(
            actual=self.aspect, reference=self.ref_aspect, precision=self.precision
        )
        self.assertRastersNoDifference(
            actual=self.aspect_threaded,
            reference=self.ref_aspect,
            precision=self.precision,
        )


class TestSlopeAspectAgainstItself(TestCase):

    precision = 0.0000001
    elevation = "elevation"

    t_aspect = "sa_together_aspect"
    t_slope = "sa_together_slope"
    t_aspect_threaded = "sa_together_aspect_threaded"
    t_slope_threaded = "sa_together_slope_threaded"

    s_aspect = "sa_separately_aspect"
    s_slope = "sa_separately_slope"
    s_aspect_threaded = "sa_separately_aspect_threaded"
    s_slope_threaded = "sa_separately_slope_threaded"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        call_module("g.region", raster="elevation")

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        call_module(
            "g.remove",
            flags="f",
            type_="raster",
            name=[
                cls.t_aspect,
                cls.t_slope,
                cls.s_slope,
                cls.s_aspect,
                cls.t_aspect_threaded,
                cls.t_slope_threaded,
                cls.s_slope_threaded,
                cls.s_aspect_threaded,
            ],
        )

    def test_slope_aspect_together(self):
        """Slope and aspect computed separately and together should be the same"""
        self.assertModule(
            "r.slope.aspect", elevation=self.elevation, aspect=self.s_aspect
        )
        self.assertModule(
            "r.slope.aspect",
            elevation=self.elevation,
            aspect=self.s_aspect_threaded,
            nprocs=8,
        )
        self.assertModule(
            "r.slope.aspect", elevation=self.elevation, slope=self.s_slope
        )
        self.assertModule(
            "r.slope.aspect",
            elevation=self.elevation,
            slope=self.s_slope_threaded,
            nprocs=8,
        )
        self.assertModule(
            "r.slope.aspect",
            elevation=self.elevation,
            slope=self.t_slope,
            aspect=self.t_aspect,
        )
        self.assertModule(
            "r.slope.aspect",
            elevation=self.elevation,
            slope=self.t_slope_threaded,
            aspect=self.t_aspect_threaded,
            nprocs=8,
        )
        self.assertRastersNoDifference(
            actual=self.t_aspect, reference=self.s_aspect, precision=self.precision
        )
        self.assertRastersNoDifference(
            actual=self.t_aspect_threaded,
            reference=self.s_aspect,
            precision=self.precision,
        )
        self.assertRastersNoDifference(
            actual=self.t_slope, reference=self.s_slope, precision=self.precision
        )
        self.assertRastersNoDifference(
            actual=self.t_slope_threaded,
            reference=self.s_slope,
            precision=self.precision,
        )


# TODO: implement this class
class TestExtremes(TestCase):

    slope = "small_slope"
    aspect = "small_aspect"
    slope_threaded = "small_slope_threaded"
    aspect_threaded = "small_aspect_threaded"
    elevation = "small_elevation"

    def setUp(self):
        self.use_temp_region()

    def tearDown(self):
        self.del_temp_region()
        call_module(
            "g.remove",
            flags="f",
            type_="raster",
            name=[
                self.slope,
                self.aspect,
                self.slope_threaded,
                self.aspect_threaded,
                self.elevation,
            ],
        )

    def test_small(self):
        self.runModule("r.in.ascii", input="-", output=self.elevation, stdin_=SMALL_MAP)
        call_module("g.region", raster=self.elevation)
        self.assertModule(
            "r.slope.aspect",
            elevation=self.elevation,
            slope=self.slope,
            aspect=self.aspect,
        )
        self.assertModule(
            "r.slope.aspect",
            elevation=self.elevation,
            slope=self.slope_threaded,
            aspect=self.aspect_threaded,
            nprocs=8,
        )
        self.assertRasterMinMax(
            map=self.slope,
            refmin=0,
            refmax=90,
            msg="Slope in degrees must be between 0 and 90",
        )
        self.assertRasterMinMax(
            map=self.slope_threaded,
            refmin=0,
            refmax=90,
            msg="Slope in degrees must be between 0 and 90",
        )
        self.assertRasterMinMax(
            map=self.aspect,
            refmin=0,
            refmax=360,
            msg="Aspect in degrees must be between 0 and 360",
        )
        self.assertRasterMinMax(
            map=self.aspect_threaded,
            refmin=0,
            refmax=360,
            msg="Aspect in degrees must be between 0 and 360",
        )


if __name__ == "__main__":
    test()
