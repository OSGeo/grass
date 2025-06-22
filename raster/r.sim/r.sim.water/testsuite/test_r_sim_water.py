import unittest

from grass.gunittest.case import TestCase


class TestRSimWater(TestCase):
    """Test r.sim.water"""

    # Set up the necessary raster maps for testing
    elevation = "elevation"
    dx = "tmp_dx"
    dy = "tmp_dy"
    depth = "tmp_depth"
    discharge = "tmp_discharge"
    rain = "tmp_rain"
    mannings = "tmp_mannings"
    infil = "tmp_infil"
    reference_depth_default = "depth_default"
    reference_discharge_default = "discharge_default"
    reference_depth_complex = "depth_complex"
    reference_discharge_complex = "discharge_complex"

    @classmethod
    def setUpClass(cls):
        """Set up region, create necessary data"""
        cls.runModule("g.region", n=224000, s=223000, e=637000, w=636000, res=10)
        cls.runModule(
            "r.slope.aspect", elevation=cls.elevation, dx=cls.dx, dy=cls.dy, flags="e"
        )
        cls.runModule(
            "r.unpack",
            input="data/depth_default.pack",
            output=cls.reference_depth_default,
        )
        cls.runModule(
            "r.unpack",
            input="data/discharge_default.pack",
            output=cls.reference_discharge_default,
        )
        cls.runModule(
            "r.random.surface", output=cls.rain, distance=200, seed=1, high=100
        )
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.mannings} = if (x() > 636500 && y() > 223500, 0.3, 0.01)",
        )
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.infil} = if (x() < 636500 && y() > 223500, 0.001, 0)",
        )
        cls.runModule(
            "r.unpack",
            input="data/depth_complex.pack",
            output=cls.reference_depth_complex,
        )
        cls.runModule(
            "r.unpack",
            input="data/discharge_complex.pack",
            output=cls.reference_discharge_complex,
        )

    @classmethod
    def tearDownClass(cls):
        """Clean up test environment"""
        cls.runModule(
            "g.remove",
            flags="f",
            type="raster",
            name=[
                cls.elevation,
                cls.dx,
                cls.dy,
                cls.reference_depth_default,
                cls.reference_discharge_default,
                cls.rain,
                cls.mannings,
                cls.infil,
                cls.reference_depth_complex,
                cls.reference_discharge_complex,
            ],
        )

    def tearDown(self):
        """Clean up test environment"""
        self.runModule(
            "g.remove",
            flags="f",
            type="raster",
            pattern=f"{self.depth}*,{self.discharge}*",
        )

    def test_default(self):
        """Test r.sim.water execution with defaults"""
        # Run the r.sim.water simulation
        self.assertModule(
            "r.sim.water",
            elevation=self.elevation,
            dx=self.dx,
            dy=self.dy,
            depth=self.depth,
            discharge=self.discharge,
            random_seed=1,
        )

        # Assert that the output rasters exist
        self.assertRasterExists(self.depth)
        self.assertRasterExists(self.discharge)
        # Assert that the output rasters are the same
        self.assertRastersEqual(
            self.depth, reference=self.reference_depth_default, precision="0.000001"
        )
        self.assertRastersEqual(
            self.discharge,
            reference=self.reference_discharge_default,
            precision="0.000001",
        )

    def test_complex(self):
        """Test r.sim.water execution with more complex inputs"""
        # Run the r.sim.water simulation
        self.assertModule(
            "r.sim.water",
            flags="t",
            elevation=self.elevation,
            dx=self.dx,
            dy=self.dy,
            rain=self.rain,
            man=self.mannings,
            infil=self.infil,
            depth=self.depth,
            discharge=self.discharge,
            niterations=15,
            output_step=5,
            diffusion_coeff=0.9,
            hmax=0.25,
            halpha=3.9,
            hbeta=0.6,
            random_seed=1,
        )

        # Assert that the output rasters exist
        self.assertRasterExists(f"{self.depth}.05")
        self.assertRasterExists(f"{self.depth}.10")
        self.assertRasterExists(f"{self.depth}.15")
        # Assert that the output rasters are the same
        self.assertRastersEqual(
            f"{self.depth}.15",
            reference=self.reference_depth_complex,
            precision="0.000001",
        )
        self.assertRastersEqual(
            f"{self.discharge}.15",
            reference=self.reference_discharge_complex,
            precision="0.000001",
        )


@unittest.skip("runs too long")
class TestRSimWaterLarge(TestCase):
    """Test r.sim.water with large region"""

    # Set up the necessary raster maps for testing
    elevation = "elevation"
    dx = "tmp_dx"
    dy = "tmp_dy"
    depth = "tmp_depth"

    @classmethod
    def setUpClass(cls):
        """Set up region, create necessary data"""
        cls.runModule("g.region", raster=cls.elevation)
        cls.runModule(
            "r.slope.aspect", elevation=cls.elevation, dx=cls.dx, dy=cls.dy, flags="e"
        )

    @classmethod
    def tearDownClass(cls):
        """Clean up test environment"""
        cls.runModule(
            "g.remove",
            flags="f",
            type="raster",
            name=[cls.elevation, cls.dx, cls.dy, cls.depth],
        )

    def test_default(self):
        """Test r.sim.water execution with defaults"""
        # Run the r.sim.water simulation
        self.assertModule(
            "r.sim.water",
            elevation=self.elevation,
            dx=self.dx,
            dy=self.dy,
            depth=self.depth,
            random_seed=1,
        )
        self.assertRasterFitsUnivar(
            self.depth, reference="sum=30423.190201", precision=1e-6
        )


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
