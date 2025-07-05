import json

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

        # Run with -p flag
        module = SimpleModule(
            "r.volume", input="elevation", clump="geology_30m", flags="p"
        )
        self.assertModule(module)
        actual_output = module.outputs.stdout.splitlines()
        self.assertEqual(actual_output, expected_output)

        # Run with format=plain
        module = SimpleModule(
            "r.volume",
            input="elevation",
            clump="geology_30m",
            format="plain",
        )
        self.assertModule(module)
        actual_output = module.outputs.stdout.splitlines()
        self.assertEqual(actual_output, expected_output)

        # Run with -p flag and format=plain
        module = SimpleModule(
            "r.volume",
            input="elevation",
            clump="geology_30m",
            flags="p",
            format="plain",
        )
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

        # Run with format=csv and -p flag (CSV output)
        module = SimpleModule(
            "r.volume", input="elevation", clump="geology_30m", flags="p", format="csv"
        )
        self.assertModule(module)
        expected_output = [
            "217,118.93,86288828,725562,635325.00,221535.00,8628882798.63",
            "262,108.97,21650560,198684,638935.00,222495.00,2165056037.02",
            "270,92.23,63578874,689373,642405.00,221485.00,6357887443.53",
            "405,132.96,33732662,253710,631835.00,224095.00,3373266208.59",
            "583,139.35,3011288,21609,630205.00,224665.00,301128821.55",
            "720,124.30,599618,4824,634075.00,227995.00,59961816.06",
            "766,132.43,936791,7074,631425.00,227845.00,93679120.08",
            "862,118.31,7302317,61722,630505.00,218885.00,730231746.74",
            "910,94.20,4235816,44964,639215.00,216365.00,423581613.11",
            "921,135.22,1693985,12528,630755.00,215445.00,169398523.05",
            "945,127.24,1145,9,630015.00,215015.00,114512.03",
            "946,89.91,365748,4068,639085.00,215255.00,36574833.85",
            "948,129.02,112632,873,630185.00,215115.00,11263181.57",
        ]
        actual_output = module.outputs.stdout.splitlines()
        self.assertEqual(actual_output, expected_output)

        # Run with format=csv and -f flag
        module = SimpleModule(
            "r.volume", input="elevation", clump="geology_30m", flags="f", format="csv"
        )
        self.assertModule(module)
        actual_output = module.outputs.stdout.splitlines()
        self.assertEqual(actual_output, expected_output)

        # Run with format=csv only
        module = SimpleModule(
            "r.volume", input="elevation", clump="geology_30m", format="csv"
        )
        self.assertModule(module)
        actual_output = module.outputs.stdout.splitlines()
        self.assertEqual(actual_output, expected_output)

    def test_separator(self):
        """Run r.volume with pipe as separator instead of default ':'"""
        module = SimpleModule(
            "r.volume",
            input="elevation",
            clump="geology_30m",
            flags="f",
            separator="pipe",
        )
        self.assertModule(module)
        expected_output = [
            "217|118.93|86288828|725562|635325.00|221535.00|8628882798.63",
            "262|108.97|21650560|198684|638935.00|222495.00|2165056037.02",
            "270|92.23|63578874|689373|642405.00|221485.00|6357887443.53",
            "405|132.96|33732662|253710|631835.00|224095.00|3373266208.59",
            "583|139.35|3011288|21609|630205.00|224665.00|301128821.55",
            "720|124.30|599618|4824|634075.00|227995.00|59961816.06",
            "766|132.43|936791|7074|631425.00|227845.00|93679120.08",
            "862|118.31|7302317|61722|630505.00|218885.00|730231746.74",
            "910|94.20|4235816|44964|639215.00|216365.00|423581613.11",
            "921|135.22|1693985|12528|630755.00|215445.00|169398523.05",
            "945|127.24|1145|9|630015.00|215015.00|114512.03",
            "946|89.91|365748|4068|639085.00|215255.00|36574833.85",
            "948|129.02|112632|873|630185.00|215115.00|11263181.57",
        ]
        actual_output = module.outputs.stdout.splitlines()
        self.assertEqual(actual_output, expected_output)

    def _assert_json_equal(self, expected_output, actual_output):
        self.assertEqual(len(expected_output), len(actual_output))
        for exp_cat, out_cat in zip(expected_output, actual_output):
            self.assertCountEqual(list(exp_cat.keys()), list(out_cat.keys()))
            for key, value in exp_cat.items():
                if isinstance(value, float):
                    self.assertAlmostEqual(value, out_cat[key], places=6)
                else:
                    self.assertEqual(value, out_cat[key])

    def test_json_format(self):
        """Run r.volume with JSON output format"""
        expected_output = [
            {
                "category": 217,
                "average": 118.92688424466569,
                "sum": 86288827.98632812,
                "cells": 725562,
                "volume": 8628882798.632812,
                "easting": 635325,
                "northing": 221535,
            },
            {
                "category": 262,
                "average": 108.96982328824218,
                "sum": 21650560.37020111,
                "cells": 198684,
                "volume": 2165056037.020111,
                "easting": 638935,
                "northing": 222495,
            },
            {
                "category": 270,
                "average": 92.22710265019957,
                "sum": 63578874.43527603,
                "cells": 689373,
                "volume": 6357887443.527603,
                "easting": 642405,
                "northing": 221485,
            },
            {
                "category": 405,
                "average": 132.95755818023326,
                "sum": 33732662.08590698,
                "cells": 253710,
                "volume": 3373266208.5906982,
                "easting": 631835,
                "northing": 224095,
            },
            {
                "category": 583,
                "average": 139.3534275323085,
                "sum": 3011288.2155456543,
                "cells": 21609,
                "volume": 301128821.5545654,
                "easting": 630205,
                "northing": 224665,
            },
            {
                "category": 720,
                "average": 124.2989553385113,
                "sum": 599618.1605529785,
                "cells": 4824,
                "volume": 59961816.05529785,
                "easting": 634075,
                "northing": 227995,
            },
            {
                "category": 766,
                "average": 132.42736794512456,
                "sum": 936791.200843811,
                "cells": 7074,
                "volume": 93679120.0843811,
                "easting": 631425,
                "northing": 227845,
            },
            {
                "category": 862,
                "average": 118.30979986652166,
                "sum": 7302317.46736145,
                "cells": 61722,
                "volume": 730231746.736145,
                "easting": 630505,
                "northing": 218885,
            },
            {
                "category": 910,
                "average": 94.20461104686294,
                "sum": 4235816.131111145,
                "cells": 44964,
                "volume": 423581613.1111145,
                "easting": 639215,
                "northing": 216365,
            },
            {
                "category": 921,
                "average": 135.21593474428315,
                "sum": 1693985.2304763794,
                "cells": 12528,
                "volume": 169398523.04763794,
                "easting": 630755,
                "northing": 215445,
            },
            {
                "category": 945,
                "average": 127.23559231228299,
                "sum": 1145.1203308105469,
                "cells": 9,
                "volume": 114512.03308105469,
                "easting": 630015,
                "northing": 215015,
            },
            {
                "category": 946,
                "average": 89.90863777168147,
                "sum": 365748.3384552002,
                "cells": 4068,
                "volume": 36574833.84552002,
                "easting": 639085,
                "northing": 215255,
            },
            {
                "category": 948,
                "average": 129.01697103319026,
                "sum": 112631.8157119751,
                "cells": 873,
                "volume": 11263181.57119751,
                "easting": 630185,
                "northing": 215115,
            },
        ]

        # Run with -p flag and format=json
        module = SimpleModule(
            "r.volume",
            input="elevation",
            clump="geology_30m",
            flags="p",
            format="json",
        )
        self.assertModule(module)
        actual_output = json.loads(module.outputs.stdout)
        self._assert_json_equal(expected_output, actual_output)

        # Run with format=json only
        module = SimpleModule(
            "r.volume",
            input="elevation",
            clump="geology_30m",
            format="json",
        )
        self.assertModule(module)
        actual_output = json.loads(module.outputs.stdout)
        self._assert_json_equal(expected_output, actual_output)


if __name__ == "__main__":
    test()
