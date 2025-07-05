from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule


class TestRVolume(TestCase):
    @classmethod
    def setUpClass(cls):
        """Set up a temporary region."""
        cls.use_temp_region()
        cls.runModule("g.region", raster="elevation")

    @classmethod
    def tearDownClass(cls):
        """Clean up the temporary region."""
        cls.del_temp_region()

    def test_plain_format(self):
        """Run r.volume with plain text output format"""

        expected_output = [
            "",
            "Volume report on data from <elevation> using clumps on <geology_30m> raster map",
            "",
            "Category   Average   Data   # Cells        Centroid             Total",
            "Number     in clump  Total  in clump   Easting     Northing     Volume",
            "-----------------------------------------------------------------------------",
            "     217    118.93  86288828  725562   635325.00   221535.00    8628882798.63",
            "     262    108.97  21650560  198684   638935.00   222495.00    2165056037.02",
            "     270     92.23  63578874  689373   642405.00   221485.00    6357887443.53",
            "     405    132.96  33732662  253710   631835.00   224095.00    3373266208.59",
            "     583    139.35   3011288   21609   630205.00   224665.00     301128821.55",
            "     720    124.30    599618    4824   634075.00   227995.00      59961816.06",
            "     766    132.43    936791    7074   631425.00   227845.00      93679120.08",
            "     862    118.31   7302317   61722   630505.00   218885.00     730231746.74",
            "     910     94.20   4235816   44964   639215.00   216365.00     423581613.11",
            "     921    135.22   1693985   12528   630755.00   215445.00     169398523.05",
            "     945    127.24      1145       9   630015.00   215015.00        114512.03",
            "     946     89.91    365748    4068   639085.00   215255.00      36574833.85",
            "     948    129.02    112632     873   630185.00   215115.00      11263181.57",
            "-----------------------------------------------------------------------------",
            "                                                Total Volume = 22351026655.81",
        ]

        # Run with default plain format
        module = SimpleModule("r.volume", input="elevation", clump="geology_30m")
        self.assertModule(module)
        actual_output = module.outputs.stdout.splitlines()
        self.assertEqual(actual_output, expected_output)

    def test_table_format(self):
        """Run r.volume with table-style output format"""

        # Run with -f flag (colon-separated output)
        module = SimpleModule(
            "r.volume", input="elevation", clump="geology_30m", flags="f"
        )
        self.assertModule(module)
        expected_output = [
            "217:118.93:86288828:725562:635325.00:221535.00:8628882798.63",
            "262:108.97:21650560:198684:638935.00:222495.00:2165056037.02",
            "270:92.23:63578874:689373:642405.00:221485.00:6357887443.53",
            "405:132.96:33732662:253710:631835.00:224095.00:3373266208.59",
            "583:139.35:3011288:21609:630205.00:224665.00:301128821.55",
            "720:124.30:599618:4824:634075.00:227995.00:59961816.06",
            "766:132.43:936791:7074:631425.00:227845.00:93679120.08",
            "862:118.31:7302317:61722:630505.00:218885.00:730231746.74",
            "910:94.20:4235816:44964:639215.00:216365.00:423581613.11",
            "921:135.22:1693985:12528:630755.00:215445.00:169398523.05",
            "945:127.24:1145:9:630015.00:215015.00:114512.03",
            "946:89.91:365748:4068:639085.00:215255.00:36574833.85",
            "948:129.02:112632:873:630185.00:215115.00:11263181.57",
        ]
        actual_output = module.outputs.stdout.splitlines()
        self.assertEqual(actual_output, expected_output)


if __name__ == "__main__":
    test()
