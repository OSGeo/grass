from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule


class TestRSunMode1Metadata(TestCase):
    """Tests the consistency of metadata when computing instantaneous solar
    incidence angle and irradiance.
    """

    elevation = "elevation"
    slope = "rsun_slope"
    aspect = "rsun_aspect"
    incidout = "incidout"

    # expected metadata with the relevant `nprocs` field
    metadata = """\"\
 ----------------------------------------------------------------\
 Day [1-365]:                              172\
 Local (solar) time (decimal hr.):         18.0000\
 Solar constant (W/m^2):                   1367.000000\
 Extraterrestrial irradiance (W/m^2):      1322.508495\
 Declination (rad):                        0.409115\
 Latitude min-max(deg):                    35.7328 - 35.7642\
 Sunrise time (hr.):                       4.79\
 Sunset time (hr.):                        19.21\
 Daylight time (hr.):                      14.43\
 Solar altitude (deg):                     13.4438\
 Solar azimuth (deg):                      289.3828\
 Linke turbidity factor:                   3.0\
 Ground albedo:                            0.200\
 -----------------------------------------------------------------\
\
r.sun --overwrite elevation="elevation" aspect="rsun_aspect" aspect_\\\
value=270.0 slope="rsun_slope" slope_value=0.0 linke_value=3.0 albed\\\
o_value=0.2 incidout="incidout" day=172 step=0.5 solar_constant=1367\\\
.0 time=18 nprocs={nprocs} distance_step=1.0 npartitions=1"\
"""

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule("g.region", n=223500, s=220000, e=640000, w=635000, res=10)
        cls.runModule(
            "r.slope.aspect",
            elevation=cls.elevation,
            slope=cls.slope,
            aspect=cls.aspect,
        )

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        cls.runModule(
            "g.remove",
            type=["raster"],
            name=[
                cls.slope,
                cls.aspect,
                cls.incidout,
            ],
            flags="f",
        )

    def setUp(self):
        self.rsun = SimpleModule(
            "r.sun",
            elevation=self.elevation,
            slope=self.slope,
            aspect=self.aspect,
            incidout=self.incidout,
            day=172,
            time=18,
            overwrite=True,
        )

    def test_more_threads(self):
        """Checks metadata correctness in threaded cases."""
        self.rsun.inputs["nprocs"].value = 4
        self.assertModule(self.rsun)
        self.assertRasterExists(name=self.incidout)
        self.assertModuleKeyValue(
            "r.info",
            map=self.incidout,
            flags="e",
            reference={"comments": self.metadata.format(nprocs=4)},
            precision=0,
            sep="=",
        )

    def test_run_outputs(self):
        """Checks metadata correctness in non-threaded cases."""
        self.assertModule(self.rsun)
        self.assertRasterExists(name=self.incidout)
        self.assertModuleKeyValue(
            "r.info",
            map=self.incidout,
            flags="e",
            reference={"comments": self.metadata.format(nprocs=1)},
            precision=0,
            sep="=",
        )


class TestRSunMode2(TestCase):
    elevation = "elevation"
    elevation_attrib = "elevation_attrib"
    elevation_threads = "elevation_threads"
    slope = "rsun_slope"
    aspect = "rsun_aspect"
    beam_rad = "beam_rad"
    glob_rad = "glob_rad"
    insol_time = "insol_time"
    beam_rad_threads = "beam_rad_threads"
    glob_rad_threads = "glob_rad_threads"
    insol_time_threads = "insol_time_threads"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule("g.region", n=223500, s=220000, e=640000, w=635000, res=10)
        cls.runModule(
            "r.slope.aspect",
            elevation=cls.elevation,
            slope=cls.slope,
            aspect=cls.aspect,
        )

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        cls.runModule(
            "g.remove",
            type=["raster"],
            name=[
                cls.slope,
                cls.aspect,
                cls.insol_time,
                cls.beam_rad,
                cls.glob_rad,
                cls.beam_rad_threads,
                cls.glob_rad_threads,
                cls.insol_time_threads,
            ],
            flags="f",
        )

    def setUp(self):
        self.rsun = SimpleModule(
            "r.sun",
            elevation=self.elevation,
            slope=self.slope,
            aspect=self.aspect,
            day=172,
            beam_rad=self.beam_rad,
            glob_rad=self.glob_rad,
            insol_time=self.insol_time,
            overwrite=True,
        )

    def test_more_threads(self):
        self.assertModule(self.rsun)
        self.rsun.inputs["nprocs"].value = 4
        self.rsun.outputs.beam_rad = self.beam_rad_threads
        self.rsun.outputs.glob_rad = self.glob_rad_threads
        self.rsun.outputs.insol_time = self.insol_time_threads
        self.assertModule(self.rsun)
        self.assertRastersNoDifference(
            self.beam_rad, self.beam_rad_threads, precision=1e-8
        )
        self.assertRastersNoDifference(
            self.glob_rad, self.glob_rad_threads, precision=1e-8
        )
        self.assertRastersNoDifference(
            self.insol_time, self.insol_time_threads, precision=1e-8
        )

    def test_run_outputs(self):
        self.assertModule(self.rsun)
        self.assertRasterExists(name=self.beam_rad)
        self.assertRasterExists(name=self.glob_rad)
        self.assertRasterExists(name=self.insol_time)
        # beam_rad
        values = "min=6561.3505859375\nmax=7680.93505859375\nmean=7638.91454059886"
        self.assertRasterFitsUnivar(
            raster=self.beam_rad, reference=values, precision=1e-8
        )
        # glob_rad
        values = "min=7829.4921875\nmax=8889.4765625\nmean=8851.4872842213"
        self.assertRasterFitsUnivar(
            raster=self.glob_rad, reference=values, precision=1e-8
        )
        # insol_time
        values = "min=11.5\nmax=14\nmean=13.8521038175691"
        self.assertRasterFitsUnivar(
            raster=self.insol_time, reference=values, precision=1e-8
        )


class TestRSunMode1(TestCase):
    elevation = "elevation"
    elevation_attrib = "elevation_attrib"
    elevation_threads = "elevation_threads"
    slope = "rsun_slope"
    aspect = "rsun_aspect"
    beam_rad = "beam_rad"
    glob_rad = "glob_rad"
    incidout = "incidout"
    beam_rad_threads = "beam_rad_threads"
    glob_rad_threads = "glob_rad_threads"
    incidout_threads = "incidout_threads"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule("g.region", n=223500, s=220000, e=640000, w=635000, res=10)
        cls.runModule(
            "r.slope.aspect",
            elevation=cls.elevation,
            slope=cls.slope,
            aspect=cls.aspect,
        )

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        cls.runModule(
            "g.remove",
            type=["raster"],
            name=[
                cls.slope,
                cls.aspect,
                cls.incidout,
                cls.beam_rad,
                cls.glob_rad,
                cls.beam_rad_threads,
                cls.glob_rad_threads,
                cls.incidout_threads,
            ],
            flags="f",
        )

    def setUp(self):
        self.rsun = SimpleModule(
            "r.sun",
            elevation=self.elevation,
            slope=self.slope,
            aspect=self.aspect,
            day=172,
            time=18,
            beam_rad=self.beam_rad,
            glob_rad=self.glob_rad,
            incidout=self.incidout,
            overwrite=True,
        )

    def test_more_threads(self):
        self.assertModule(self.rsun)
        self.rsun.inputs["nprocs"].value = 4
        self.rsun.outputs.beam_rad = self.beam_rad_threads
        self.rsun.outputs.glob_rad = self.glob_rad_threads
        self.rsun.outputs.incidout = self.incidout_threads
        self.assertModule(self.rsun)
        self.assertRastersNoDifference(
            self.beam_rad, self.beam_rad_threads, precision=1e-8
        )
        self.assertRastersNoDifference(
            self.glob_rad, self.glob_rad_threads, precision=1e-8
        )
        self.assertRastersNoDifference(
            self.incidout, self.incidout_threads, precision=1e-8
        )

    def test_run_outputs(self):
        self.assertModule(self.rsun)
        self.assertRasterExists(name=self.beam_rad)
        self.assertRasterExists(name=self.glob_rad)
        self.assertRasterExists(name=self.incidout)
        # beam_rad
        values = "min=0\nmax=355.780944824219\nmean=124.615522080923"
        self.assertRasterFitsUnivar(
            raster=self.beam_rad, reference=values, precision=1e-8
        )
        # glob_rad
        values = "min=32.1095008850098\nmax=451.527923583984\nmean=178.057346498427"
        self.assertRasterFitsUnivar(
            raster=self.glob_rad, reference=values, precision=1e-8
        )
        # incidout
        values = "min=0.0100772567093372\nmax=40.5435371398926\nmean=13.2855086254291"
        self.assertRasterFitsUnivar(
            raster=self.incidout, reference=values, precision=1e-8
        )


class TestRSunHighNorthernSlope(TestCase):
    elevation = "elevation_high_slope"
    slope = "slope_high_slope"
    aspect = "aspect_high_slope"
    beam_radiation = "beam_high_slope"

    @classmethod
    def setUpClass(cls):
        """Use temporary region settings"""
        cls.use_temp_region()
        cls.runModule("g.region", raster="elevation", res=50, flags="a")
        cls.runModule(
            "r.mapcalc",
            expression=(
                f"{cls.elevation} = "
                "-0.0002 * (x() - 637500) * (x() - 637500) +"
                "0.0002 * (y() - 221750) * (y() - 221750)"
            ),
        )
        cls.runModule(
            "r.slope.aspect",
            elevation=cls.elevation,
            slope=cls.slope,
            aspect=cls.aspect,
        )

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region"""
        cls.del_temp_region()
        cls.runModule(
            "g.remove",
            type="raster",
            flags="f",
            name=[cls.elevation, cls.aspect, cls.slope],
        )

    def tearDown(self):
        self.runModule("g.remove", type="raster", flags="f", name=self.beam_radiation)

    def test_beam_radiation_no_terrain(self):
        self.assertModule(
            "r.sun",
            flags="p",
            elevation=self.elevation,
            aspect=self.aspect,
            slope=self.slope,
            beam_rad=self.beam_radiation,
            day=90,
        )
        self.assertRasterExists(name=self.beam_radiation)
        values = "min=50.77406\nmax=6845.00146\nmean=3452.85688\nstddev=1893.56956"
        self.assertRasterFitsUnivar(
            raster=self.beam_radiation, reference=values, precision=1e-5
        )

    def test_beam_radiation_with_terrain(self):
        self.assertModule(
            "r.sun",
            elevation=self.elevation,
            aspect=self.aspect,
            slope=self.slope,
            beam_rad=self.beam_radiation,
            day=90,
        )
        self.assertRasterExists(name=self.beam_radiation)
        values = "min=44.05826\nmax=6845.00146\nmean=3425.53184\nstddev=1891.98052"
        self.assertRasterFitsUnivar(
            raster=self.beam_radiation, reference=values, precision=1e-5
        )


if __name__ == "__main__":
    test()
