import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestVOverlay(TestCase):
    """Test v.overlay output against expected output"""

    @classmethod
    def setUpClass(cls):
        """Create input vectors for v.overlay."""
        gs.run_command(
            "v.extract",
            input="boundary_county",
            output="boundary_county_extract1",
            where="NAME in ('CURRITUCK', 'COLUMBUS')",
        )
        gs.run_command(
            "v.extract",
            input="boundary_county",
            output="boundary_county_extract2",
            where="NAME not in ('CURRITUCK', 'COLUMBUS')",
        )
        # modify extract 1
        gs.run_command(
            "v.buffer",
            input="boundary_county_extract1",
            output="boundary_county_extract1_buffer_out",
            type="area",
            distance=2,
        )
        gs.run_command(
            "v.buffer",
            input="boundary_county_extract1_buffer_out",
            output="boundary_county_extract1_buffer_in",
            type="area",
            distance=-2,
        )

    @classmethod
    def tearDownClass(cls):
        """Remove created vector map and temporary files, then delete the temp region."""
        gs.run_command(
            "g.remove", type="vector", flags="f", pattern="boundary_county_extract*"
        )

    def test_voverlay_nocleaning(self):
        """
        Overlay two input vectors and compare the output to the expected output.
        The output will have many very small areas.
        This test would fail"""
        gs.run_command(
            "v.overlay",
            ainput="boundary_county_extract1_buffer_in",
            atype="area",
            binput="boundary_county_extract2",
            btype="area",
            output="boundary_county_extract_overlay_nocleaning",
            operator="or",
            snap=-1,
            quiet=True,
        )

        self.assertModuleKeyValue(
            "v.info",
            map="boundary_county_extract_overlay_nocleaning",
            flags="tg",
            sep="=",
            precision=0.1,
            reference={
                "nodes": 12067,
                "primitives": 21060,
                "points": 0,
                "lines": 0,
                "boundaries": 18053,
                "centroids": 3007,
                "areas": 6156,
                "islands": 170,
            },
        )

    def test_voverlay_withcleaning(self):
        """Overlay two input vectors and compare the output to the expected output.
        The output will have many very small areas"""
        gs.run_command(
            "v.overlay",
            ainput="boundary_county_extract1_buffer_in",
            atype="area",
            binput="boundary_county_extract2",
            btype="area",
            output="boundary_county_extract_overlay_withcleaning",
            operator="or",
            snap=1.0e-6,
            minsize=0.00001,
            quiet=True,
        )

        self.assertModuleKeyValue(
            "v.info",
            map="boundary_county_extract_overlay_withcleaning",
            flags="tg",
            sep="=",
            precision=0.1,
            reference={
                "nodes": 13733,
                "primitives": 24393,
                "points": 0,
                "lines": 0,
                "boundaries": 20563,
                "centroids": 3830,
                "areas": 6999,
                "islands": 169,
            },
        )


if __name__ == "__main__":
    test()
