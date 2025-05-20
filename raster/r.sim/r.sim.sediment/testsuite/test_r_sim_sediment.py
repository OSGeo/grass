from grass.gunittest.case import TestCase


class TestRSimSediment(TestCase):
    """Test r.sim.sediment"""

    # Set up the necessary raster maps for testing
    elevation = "elevation"
    dx = "tmp_dx"
    dy = "tmp_dy"
    depth_default = "depth_default"
    tranin = "tranin"
    detin = "detin"
    tauin = "tauin"
    sedflux = "sedflux"
    erdep = "erdep"
    reference_sedflux = "reference_sedflux"
    reference_erdep = "reference_erdep"

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
            output=cls.depth_default,
        )
        cls.runModule("r.mapcalc", expression=f"{cls.tranin} = 0.001")
        cls.runModule("r.mapcalc", expression=f"{cls.detin} = 0.001")
        cls.runModule("r.mapcalc", expression=f"{cls.tauin} = 0.01")
        cls.runModule(
            "r.unpack",
            input="data/reference_sedflux.pack",
            output=cls.reference_sedflux,
        )
        cls.runModule(
            "r.unpack", input="data/reference_erdep.pack", output=cls.reference_erdep
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
                cls.depth_default,
                cls.tranin,
                cls.detin,
                cls.tauin,
                cls.reference_sedflux,
                cls.reference_erdep,
            ],
        )

    def tearDown(self):
        """Clean up test environment"""
        self.runModule(
            "g.remove",
            flags="f",
            type="raster",
            name=f"{self.sedflux},{self.erdep}",
        )

    def test_default(self):
        """Test r.sim.sediment execution with defaults"""
        # Run the r.sim.sediment simulation
        self.assertModule(
            "r.sim.sediment",
            elevation=self.elevation,
            dx=self.dx,
            dy=self.dy,
            water_depth=self.depth_default,
            man_value=0.05,
            detachment_coeff=self.detin,
            transport_coeff=self.tranin,
            shear_stress=self.tauin,
            sediment_flux=self.sedflux,
            erosion_deposition=self.erdep,
            random_seed=1,
        )

        # Assert that the output rasters exist
        self.assertRasterExists(self.sedflux)
        self.assertRasterExists(self.erdep)
        # Assert that the output rasters are the same
        self.assertRastersEqual(
            self.sedflux, reference=self.reference_sedflux, precision="0.000001"
        )
        self.assertRastersEqual(
            self.erdep,
            reference=self.reference_erdep,
            precision="0.000001",
        )


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
