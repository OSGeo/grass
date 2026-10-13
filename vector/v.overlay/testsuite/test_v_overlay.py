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
            where="NAME in ('CURRITUCK')",
        )
        gs.run_command(
            "v.extract",
            input="boundary_county",
            output="boundary_county_extract2",
            where="NAME in ('CAMDEN')",
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
        This test would fail in GRASS84 with a too small number of centroids.
        Since GRASS84, calculation for centroids has been improved for
        edge cases like very small areas."""
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
                "nodes": 1050,
                "primitives": 1802,
                "points": 0,
                "lines": 0,
                "boundaries": 1572,
                "centroids": 230,
                "areas": 529,
                "islands": 7,
            },
        )

    def test_voverlay_withcleaning(self):
        """Overlay two input vectors and compare the output to the expected output.
        The output will have many very small areas.
        This is a test for the cleaning options snap and minsize"""
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
                "nodes": 1232,
                "primitives": 2164,
                "points": 0,
                "lines": 0,
                "boundaries": 1847,
                "centroids": 317,
                "areas": 622,
                "islands": 7,
            },
        )


if __name__ == "__main__":
    test()
