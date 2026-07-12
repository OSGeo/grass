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
    precision = 1e-14
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
            "r.in.gdal", flags="o", input="data/gdal_slope.ascii", output=self.ref_slope
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
            "r.in.gdal",
            flags="o",
            input="data/gdal_aspect.ascii",
            output=self.ref_aspect,
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
            actual=self.aspect,
            reference=self.ref_aspect,
            precision=0.0001,  # TODO: Explore why a larger threshold is needed rather than self.precision
        )
        self.assertRastersNoDifference(
            actual=self.aspect_threaded,
            reference=self.ref_aspect,
            precision=0.0001,  # TODO: Explore why a larger threshold is needed rather than self.precision
        )


class TestSlopeAspectAgainstItself(TestCase):
    precision = 1e-14
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


class TestSlopeAspectEdge(TestCase):
    """Test -e flag on slope. Only tests results didn't change between
    serial and parallelized version.
    """

    # precision for comparisons
    precision = 1e-14
    slope = "elevation_slope"
    slope_threaded = "elevation_slope_threaded"
    slope_edge = "elevation_slope_edge"
    slope_threaded_edge = "elevation_slope_threaded_edge"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.elevation = "elevation@PERMANENT"
        call_module("g.region", raster=cls.elevation)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        cls.runModule(
            "g.remove",
            flags="f",
            type="raster",
            name=[
                cls.slope,
                cls.slope_threaded,
                cls.slope_edge,
                cls.slope_threaded_edge,
            ],
        )

    def test_slope(self):
        self.assertModule("r.slope.aspect", elevation=self.elevation, slope=self.slope)
        self.assertModule(
            "r.slope.aspect",
            elevation=self.elevation,
            slope=self.slope_threaded,
            nprocs=8,
        )
        values = "null_cells=5696\nmean=3.86452240667335"
        self.assertRasterFitsUnivar(
            raster=self.slope, reference=values, precision=self.precision
        )
        self.assertRasterFitsUnivar(
            raster=self.slope_threaded, reference=values, precision=self.precision
        )
        # check we have expected values
        self.assertModule(
            "r.slope.aspect", elevation=self.elevation, slope=self.slope_edge, flags="e"
        )
        self.assertModule(
            "r.slope.aspect",
            elevation=self.elevation,
            slope=self.slope_threaded_edge,
            flags="e",
            nprocs=8,
        )
        values = "null_cells=0\nmean=3.86118542369876"
        self.assertRasterFitsUnivar(
            raster=self.slope_edge, reference=values, precision=self.precision
        )
        self.assertRasterFitsUnivar(
            raster=self.slope_threaded_edge, reference=values, precision=self.precision
        )


class TestSlopeAspectAllOutputs(TestCase):
    """Test all outputs. Only tests results didn't change between
    serial and parallelized version.
    """

    # precision for comparisons
    precision = 1e-11
    slope = "elevation_slope"
    slope_threaded = "elevation_slope_threaded"
    aspect = "elevation_aspect"
    aspect_threaded = "elevation_aspect_threaded"
    pcurvature = "elevation_pcurvature"
    pcurvature_threaded = "elevation_pcurvature_threaded"
    tcurvature = "elevation_tcurvature"
    tcurvature_threaded = "elevation_tcurvature_threaded"
    dx = "elevation_dx"
    dx_threaded = "elevation_dx_threaded"
    dy = "elevation_dy"
    dy_threaded = "elevation_dy_threaded"
    dxx = "elevation_dxx"
    dxx_threaded = "elevation_dxx_threaded"
    dyy = "elevation_dyy"
    dyy_threaded = "elevation_dyy_threaded"
    dxy = "elevation_dxy"
    dxy_threaded = "elevation_dxy_threaded"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.elevation = "elevation@PERMANENT"
        call_module("g.region", raster=cls.elevation)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        cls.runModule(
            "g.remove",
            flags="f",
            type="raster",
            name=[
                cls.slope,
                cls.slope_threaded,
                cls.aspect,
                cls.aspect_threaded,
                cls.pcurvature,
                cls.pcurvature_threaded,
                cls.tcurvature,
                cls.tcurvature_threaded,
                cls.dx,
                cls.dx_threaded,
                cls.dy,
                cls.dy_threaded,
                cls.dxx,
                cls.dxx_threaded,
                cls.dyy,
                cls.dyy_threaded,
                cls.dxy,
                cls.dxy_threaded,
            ],
        )

    def test_slope(self):
        self.assertModule("r.slope.aspect", elevation=self.elevation, slope=self.slope)
        self.assertModule(
            "r.slope.aspect",
            elevation=self.elevation,
            slope=self.slope_threaded,
            nprocs=8,
        )
        values = "mean=3.86452240667335\nrange=38.6893920898438"
        self.assertRasterFitsUnivar(
            raster=self.slope, reference=values, precision=self.precision
        )
        self.assertRasterFitsUnivar(
            raster=self.slope_threaded, reference=values, precision=self.precision
        )

    def test_aspect(self):
        self.assertModule(
            "r.slope.aspect", elevation=self.elevation, aspect=self.aspect
        )
        self.assertModule(
            "r.slope.aspect",
            elevation=self.elevation,
            aspect=self.aspect_threaded,
            nprocs=8,
        )
        values = "mean=190.022878119363\nrange=360"
        self.assertRasterFitsUnivar(
            raster=self.aspect, reference=values, precision=self.precision
        )
        self.assertRasterFitsUnivar(
            raster=self.aspect_threaded, reference=values, precision=self.precision
        )

    def test_pcurvature(self):
        self.assertModule(
            "r.slope.aspect", elevation=self.elevation, pcurvature=self.pcurvature
        )
        self.assertModule(
            "r.slope.aspect",
            elevation=self.elevation,
            pcurvature=self.pcurvature_threaded,
            nprocs=8,
        )
        values = "mean=-8.11389945677247e-06\nrange=0.18258623033762"
        self.assertRasterFitsUnivar(
            raster=self.pcurvature, reference=values, precision=self.precision
        )
        self.assertRasterFitsUnivar(
            raster=self.pcurvature_threaded, reference=values, precision=self.precision
        )

    def test_tcurvature(self):
        self.assertModule(
            "r.slope.aspect", elevation=self.elevation, tcurvature=self.tcurvature
        )
        self.assertModule(
            "r.slope.aspect",
            elevation=self.elevation,
            tcurvature=self.tcurvature_threaded,
            nprocs=8,
        )
        values = "mean=9.98676512087167e-06\nrange=0.173980213701725"
        self.assertRasterFitsUnivar(
            raster=self.tcurvature, reference=values, precision=self.precision
        )
        self.assertRasterFitsUnivar(
            raster=self.tcurvature_threaded, reference=values, precision=self.precision
        )

    def test_dx(self):
        self.assertModule("r.slope.aspect", elevation=self.elevation, dx=self.dx)
        self.assertModule(
            "r.slope.aspect",
            elevation=self.elevation,
            dx=self.dx_threaded,
            nprocs=8,
        )
        values = "mean=0.00298815584231336\nrange=1.22372794151306"
        self.assertRasterFitsUnivar(
            raster=self.dx, reference=values, precision=self.precision
        )
        self.assertRasterFitsUnivar(
            raster=self.dx_threaded, reference=values, precision=self.precision
        )

    def test_dy(self):
        self.assertModule("r.slope.aspect", elevation=self.elevation, dy=self.dy)
        self.assertModule(
            "r.slope.aspect",
            elevation=self.elevation,
            dy=self.dy_threaded,
            nprocs=8,
        )
        values = "mean=-0.000712442231985616\nrange=1.43247389793396"
        self.assertRasterFitsUnivar(
            raster=self.dy, reference=values, precision=self.precision
        )
        self.assertRasterFitsUnivar(
            raster=self.dy_threaded, reference=values, precision=self.precision
        )

    def test_dxx(self):
        self.assertModule("r.slope.aspect", elevation=self.elevation, dxx=self.dxx)
        self.assertModule(
            "r.slope.aspect",
            elevation=self.elevation,
            dxx=self.dxx_threaded,
            nprocs=8,
        )
        values = "mean=1.3698233535033e-06\nrange=0.211458221077919"
        self.assertRasterFitsUnivar(
            raster=self.dxx, reference=values, precision=self.precision
        )
        self.assertRasterFitsUnivar(
            raster=self.dxx_threaded, reference=values, precision=self.precision
        )

    def test_dyy(self):
        self.assertModule("r.slope.aspect", elevation=self.elevation, dyy=self.dyy)
        self.assertModule(
            "r.slope.aspect",
            elevation=self.elevation,
            dyy=self.dyy_threaded,
            nprocs=8,
        )
        values = "mean=1.58118857641799e-06\nrange=0.217463649809361"
        self.assertRasterFitsUnivar(
            raster=self.dyy, reference=values, precision=self.precision
        )
        self.assertRasterFitsUnivar(
            raster=self.dyy_threaded, reference=values, precision=self.precision
        )

    def test_dxy(self):
        self.assertModule("r.slope.aspect", elevation=self.elevation, dxy=self.dxy)
        self.assertModule(
            "r.slope.aspect",
            elevation=self.elevation,
            dxy=self.dxy_threaded,
            nprocs=8,
        )
        values = "mean=2.07370162614472e-07\nrange=0.0772973857820034"
        self.assertRasterFitsUnivar(
            raster=self.dxy, reference=values, precision=self.precision
        )
        self.assertRasterFitsUnivar(
            raster=self.dxy_threaded, reference=values, precision=self.precision
        )


if __name__ == "__main__":
    test()
