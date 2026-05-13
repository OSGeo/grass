import pytest
import grass.script as gs

# run in a GRASS session with nc_spm_full_v2beta1


class TestVOverlay:
    """Test v.overlay output against expected output"""

    # create test data
    @pytest.fixture(scope="class", autouse=True)
    def create_testdata(self):
        # set up
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

        # run the tests
        yield

        # clean up test data regardless of test success/failure
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

        reference = {
            "nodes": 1050,
            "primitives": 1802,
            "points": 0,
            "lines": 0,
            "boundaries": 1572,
            "centroids": 230,
            "areas": 529,
            "islands": 7,
        }

        observed = gs.vector_info_topo(
            map="boundary_county_extract_overlay_nocleaning",
        )

        # compare results
        for key in list(reference):
            # all integers, simple comparison is ok
            assert reference[key] == observed[key], (
                f"Difference in {key}, expected {reference[key]}, got {observed[key]}"
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

        reference = {
            "nodes": 1232,
            "primitives": 2164,
            "points": 0,
            "lines": 0,
            "boundaries": 1847,
            "centroids": 317,
            "areas": 622,
            "islands": 7,
        }

        observed = gs.vector_info_topo(
            map="boundary_county_extract_overlay_withcleaning",
        )

        # compare results
        for key in list(reference):
            # all integers, simple comparison is ok
            assert reference[key] == observed[key], (
                f"Difference in {key}, expected {reference[key]}, got {observed[key]}"
            )
