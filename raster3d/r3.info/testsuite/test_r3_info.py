from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.script.core import parse_command, read_command


class TestR3Info(TestCase):
    """Unit tests for the r3.info module"""

    test_raster3d = "test_raster3d"

    @classmethod
    def setUpClass(cls):
        """Create a temporary region and generate a test raster_3d map."""
        cls.use_temp_region()
        cls.runModule("g.region", n=200, s=100, e=400, w=200, t=500, b=450, res3=1)
        cls.runModule(
            "r3.mapcalc",
            expression="%s = row() + col() + depth()" % cls.test_raster3d,
        )

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region and delete the test raster_3d map."""
        cls.del_temp_region()
        cls.runModule(
            "g.remove",
            flags="f",
            type="raster_3d",
            name=cls.test_raster3d,
        )

    def _assert_json_equal(self, result, expected, check_variable_fields=True):
        # If requested, verify that the fields which vary with the Grass sample
        # data's path exist in the JSON output and not exact values
        if check_variable_fields:
            remove_fields = ["project", "creator", "database", "date", "mapset"]
            for field in remove_fields:
                self.assertIn(field, result)
                result.pop(field)

        self.assertCountEqual(list(expected.keys()), list(result.keys()))

        for key, value in expected.items():
            if isinstance(value, float):
                self.assertAlmostEqual(value, result[key])
            else:
                self.assertEqual(value, result[key])

    def test_plain_output_default(self):
        """Verify plain text output without flags."""
        result = read_command("r3.info", map=self.test_raster3d).splitlines()
        expected = [
            " +----------------------------------------------------------------------------+",
            " | Map:      test_raster3d                  Date:                             |",
            " | Mapset:                                  Login of Creator:                 |",
            " | Project:                                                                   |",
            " | DataBase:                                                                  |",
            " | Title:    test_raster3d                                                    |",
            " | Units:    none                                                             |",
            " | Vertical unit: units                                                       |",
            " | Timestamp: none                                                            |",
            " |----------------------------------------------------------------------------|",
            " |                                                                            |",
            " |   Type of Map:  raster_3d            Number of Categories: 0               |",
            " |   Data Type:    DCELL                                                      |",
            " |   Rows:         100                                                        |",
            " |   Columns:      200                                                        |",
            " |   Depths:       50                                                         |",
            " |   Total Cells:  1000000                                                    |",
            " |   Total size:           147591 Bytes                                       |",
            " |   Number of tiles:      294                                                |",
            " |   Mean tile size:       502 Bytes                                          |",
            " |   Tile size in memory:  31320 Bytes                                        |",
            " |   Number of tiles in x, y and  z:   7, 7, 6                                |",
            " |   Dimension of a tile in x, y, z:   29, 15, 9                              |",
            " |                                                                            |",
            " |        Projection: Lambert Conformal Conic (zone 0)                        |",
            " |            N:        200    S:        100   Res:     1                     |",
            " |            E:        400    W:        200   Res:     1                     |",
            " |            T:        500    B:        450   Res:     1                     |",
            " |   Range of data:   min =          3 max =        350                       |",
            " |                                                                            |",
            " |   Data Source:                                                             |",
            " |                                                                            |",
            " |                                                                            |",
            " |                                                                            |",
            " |   Data Description:                                                        |",
            " |    generated by r3.mapcalc                                                 |",
            " |                                                                            |",
            " |   Comments:                                                                |",
            ' |    r3.mapcalc expression="test_raster3d = row() + col() + depth()" regi\\   |',
            ' |    on="current" nprocs=0                                                   |',
            " |                                                                            |",
            " +----------------------------------------------------------------------------+",
            "",
        ]
        # Skip exact match for lines containing "Project:", "Date:", "Login of Creator:",
        # "DataBase:" or "Mapset:" because their values vary
        for i, component in enumerate(result):
            if (
                "Project:" in component
                or "DataBase:" in component
                or "Login of Creator:" in component
                or "Date:" in component
                or "Mapset:" in component
            ):
                continue
            self.assertEqual(component, expected[i], f"Mismatch at line {i + 1}")

        # Verify with explicit plain format
        result_plain = read_command(
            "r3.info", map=self.test_raster3d, format="plain"
        ).splitlines()
        self.assertEqual(result_plain, result)

    def test_shell_output_default(self):
        """Verify shell format output without flags."""
        result = read_command(
            "r3.info", map=self.test_raster3d, format="shell"
        ).splitlines()
        expected = [
            "map=test_raster3d",
            "date=",
            "mapset=",
            "creator=",
            "project=",
            "database=",
            'title="test_raster3d"',
            'units="none"',
            'vertical_units="units"',
            'timestamp="none"',
            "maptype=raster_3d",
            "ncats=0",
            'datatype="DCELL"',
            "rows=100",
            "cols=200",
            "depths=50",
            "cells=1000000",
            "size=147591",
            "ntiles=294",
            "meansize=502",
            "tilesize=31320",
            "tilenumx=7",
            "tilenumy=7",
            "tilenumz=6",
            "tiledimx=29",
            "tiledimy=15",
            "tiledimz=9",
            "north=200",
            "south=100",
            "nsres=1",
            "east=400",
            "west=200",
            "ewres=1",
            "top=500",
            "bottom=450",
            "tbres=1",
            "min=3",
            "max=350",
            'source1=""',
            'source2=""',
            'description="generated by r3.mapcalc"',
            'comments="r3.mapcalc expression="test_raster3d = row() + col() + depth()" regi\\on="current" nprocs=0"',
        ]
        # Skip exact match for lines containing "project", "date", "creator", "database", "mapset" because
        # their values vary
        for i, component in enumerate(result):
            if any(
                keyword in component
                for keyword in ("project", "creator", "database", "date", "mapset")
            ):
                continue
            self.assertEqual(component, expected[i], f"Mismatch at line {i + 1}")

    def test_json_output_default(self):
        """Verify JSON output without flags."""
        result = parse_command("r3.info", map=self.test_raster3d, format="json")
        expected = {
            "map": "test_raster3d",
            "title": "test_raster3d",
            "units": "none",
            "vertical_units": "units",
            "timestamp": None,
            "maptype": "raster_3d",
            "ncats": 0,
            "datatype": "DCELL",
            "rows": 100,
            "cols": 200,
            "depths": 50,
            "cells": 1000000,
            "size": 147591,
            "ntiles": 294,
            "meansize": 502,
            "tilesize": 31320,
            "tilenumx": 7,
            "tilenumy": 7,
            "tilenumz": 6,
            "tiledimx": 29,
            "tiledimy": 15,
            "tiledimz": 9,
            "north": 200,
            "south": 100,
            "nsres": 1,
            "east": 400,
            "west": 200,
            "ewres": 1,
            "top": 500,
            "bottom": 450,
            "tbres": 1,
            "min": 3,
            "max": 350,
            "source1": "",
            "source2": "",
            "description": "generated by r3.mapcalc",
            "comments": 'r3.mapcalc expression="test_raster3d = row() + col() + depth()" region="current" nprocs=0',
        }
        self._assert_json_equal(result, expected)

    def test_shell_output_with_g_flag(self):
        """Verify shell format output with -g flag only."""
        result = read_command("r3.info", map=self.test_raster3d, flags="g").splitlines()
        expected = [
            "north=200",
            "south=100",
            "east=400",
            "west=200",
            "bottom=450",
            "top=500",
            "nsres=1",
            "ewres=1",
            "tbres=1",
            "rows=100",
            "cols=200",
            "depths=50",
            'datatype="DCELL"',
            'timestamp="none"',
            'units="none"',
            'vertical_units="units"',
            "tilenumx=7",
            "tilenumy=7",
            "tilenumz=6",
            "tiledimx=29",
            "tiledimy=15",
            "tiledimz=9",
        ]
        self.assertEqual(result, expected)

        # Verify with explicit shell format
        result = read_command(
            "r3.info", map=self.test_raster3d, flags="g", format="shell"
        ).splitlines()
        self.assertEqual(result, expected)

    def test_plain_output_with_g_flag(self):
        """Verify plain text output with -g flag only."""
        result = read_command(
            "r3.info", map=self.test_raster3d, flags="g", format="plain"
        ).splitlines()
        expected = [
            "North: 200",
            "South: 100",
            "East: 400",
            "West: 200",
            "Bottom: 450",
            "Top: 500",
            "North-south resolution: 1",
            "East-west resolution: 1",
            "Top-Bottom resolution: 1",
            "Rows: 100",
            "Columns: 200",
            "Depths: 50",
            "Data Type: DCELL",
            "Timestamp: none",
            "Units: none",
            "Vertical unit: units",
            "Number of tiles in x: 7",
            "Number of tiles in y: 7",
            "Number of tiles in z: 6",
            "Dimension of a tile in x: 29",
            "Dimension of a tile in y: 15",
            "Dimension of a tile in z: 9",
        ]
        self.assertEqual(result, expected)

    def test_json_output_with_g_flag(self):
        """Verify JSON output with -g flag only."""
        result = parse_command(
            "r3.info", map=self.test_raster3d, flags="g", format="json"
        )
        expected = {
            "north": 200,
            "south": 100,
            "east": 400,
            "west": 200,
            "bottom": 450,
            "top": 500,
            "nsres": 1,
            "ewres": 1,
            "tbres": 1,
            "rows": 100,
            "cols": 200,
            "depths": 50,
            "datatype": "DCELL",
            "timestamp": None,
            "units": "none",
            "vertical_units": "units",
            "tilenumx": 7,
            "tilenumy": 7,
            "tilenumz": 6,
            "tiledimx": 29,
            "tiledimy": 15,
            "tiledimz": 9,
        }
        self._assert_json_equal(result, expected, False)

    def test_shell_output_with_r_flag(self):
        """Verify shell output with -r flag only."""
        result = read_command("r3.info", map=self.test_raster3d, flags="r").splitlines()
        expected = ["min=3.000000", "max=350.000000"]
        self.assertEqual(result, expected)

        # Verify with explicit shell format
        result = read_command(
            "r3.info", map=self.test_raster3d, flags="r", format="shell"
        ).splitlines()
        self.assertEqual(result, expected)

    def test_plain_output_with_r_flag(self):
        """Verify plain output with -r flag only."""
        result = read_command(
            "r3.info", map=self.test_raster3d, flags="r", format="plain"
        ).splitlines()
        expected = ["Minimum: 3.000000", "Maximum: 350.000000"]
        self.assertEqual(result, expected)

    def test_json_output_with_r_flag(self):
        """Verify JSON output with -r flag only."""
        result = parse_command(
            "r3.info", map=self.test_raster3d, flags="r", format="json"
        )
        expected = {"min": 3, "max": 350}
        self._assert_json_equal(result, expected, False)

    def test_shell_output_with_h_flag(self):
        """Verify shell output with -h flag only."""
        result = read_command(
            "r3.info", map=self.test_raster3d, flags="h", format="shell"
        ).splitlines()
        expected = [
            'title="test_raster3d"',
            'source1=""',
            'source2=""',
            'description="generated by r3.mapcalc"',
            'comments="r3.mapcalc expression="test_raster3d = row() + col() + depth()" regi\\on="current" nprocs=0"',
        ]
        self.assertEqual(result, expected)

    def test_plain_output_with_h_flag(self):
        """Verify plain output with -h flag only."""
        result = read_command(
            "r3.info",
            map=self.test_raster3d,
            flags="h",
        ).splitlines()
        expected = [
            "Title:",
            "   test_raster3d",
            "Data Source:",
            "   ",
            "   ",
            "Data Description:",
            "   generated by r3.mapcalc",
            "Comments:",
            '   r3.mapcalc expression="test_raster3d = row() + col() + depth()" regi\\',
            '   on="current" nprocs=0',
        ]
        self.assertEqual(result, expected)

        # Verify with explicit plain format
        result = read_command(
            "r3.info", map=self.test_raster3d, flags="h", format="plain"
        ).splitlines()
        self.assertEqual(result, expected)

    def test_json_output_with_h_flag(self):
        """Verify JSON output with -h flag only."""
        result = parse_command(
            "r3.info", map=self.test_raster3d, flags="h", format="json"
        )
        expected = {
            "title": "test_raster3d",
            "source1": "",
            "source2": "",
            "description": "generated by r3.mapcalc",
            "comments": 'r3.mapcalc expression="test_raster3d = row() + col() + depth()" region="current" nprocs=0',
        }
        self._assert_json_equal(result, expected, False)

    def test_combined_flags_shell_output(self):
        """Verify shell output with all flags (-grh flag)."""
        result = read_command(
            "r3.info",
            map=self.test_raster3d,
            flags="grh",
        ).splitlines()
        expected = [
            "north=200",
            "south=100",
            "east=400",
            "west=200",
            "bottom=450",
            "top=500",
            "nsres=1",
            "ewres=1",
            "tbres=1",
            "rows=100",
            "cols=200",
            "depths=50",
            'datatype="DCELL"',
            'timestamp="none"',
            'units="none"',
            'vertical_units="units"',
            "tilenumx=7",
            "tilenumy=7",
            "tilenumz=6",
            "tiledimx=29",
            "tiledimy=15",
            "tiledimz=9",
            "min=3.000000",
            "max=350.000000",
            'title="test_raster3d"',
            'source1=""',
            'source2=""',
            'description="generated by r3.mapcalc"',
            'comments="r3.mapcalc expression="test_raster3d = row() + col() + depth()" regi\\on="current" nprocs=0"',
        ]
        self.assertEqual(result, expected)

        # Verify with explicit shell format
        result = read_command(
            "r3.info", map=self.test_raster3d, flags="grh", format="shell"
        ).splitlines()
        self.assertEqual(result, expected)

    def test_combined_flags_plain_output(self):
        """Verify plain output with all flags (-grh flag)."""
        result = read_command(
            "r3.info", map=self.test_raster3d, flags="grh", format="plain"
        ).splitlines()
        expected = [
            "North: 200",
            "South: 100",
            "East: 400",
            "West: 200",
            "Bottom: 450",
            "Top: 500",
            "North-south resolution: 1",
            "East-west resolution: 1",
            "Top-Bottom resolution: 1",
            "Rows: 100",
            "Columns: 200",
            "Depths: 50",
            "Data Type: DCELL",
            "Timestamp: none",
            "Units: none",
            "Vertical unit: units",
            "Number of tiles in x: 7",
            "Number of tiles in y: 7",
            "Number of tiles in z: 6",
            "Dimension of a tile in x: 29",
            "Dimension of a tile in y: 15",
            "Dimension of a tile in z: 9",
            "Minimum: 3.000000",
            "Maximum: 350.000000",
            "Title:",
            "   test_raster3d",
            "Data Source:",
            "   ",
            "   ",
            "Data Description:",
            "   generated by r3.mapcalc",
            "Comments:",
            '   r3.mapcalc expression="test_raster3d = row() + col() + depth()" regi\\',
            '   on="current" nprocs=0',
        ]
        self.assertEqual(result, expected)

    def test_combined_flags_json_output(self):
        """Verify JSON output with all flags (-grh flag)."""
        result = parse_command(
            "r3.info", map=self.test_raster3d, flags="grh", format="json"
        )
        expected = {
            "north": 200,
            "south": 100,
            "east": 400,
            "west": 200,
            "bottom": 450,
            "top": 500,
            "nsres": 1,
            "ewres": 1,
            "tbres": 1,
            "rows": 100,
            "cols": 200,
            "depths": 50,
            "datatype": "DCELL",
            "timestamp": None,
            "units": "none",
            "vertical_units": "units",
            "tilenumx": 7,
            "tilenumy": 7,
            "tilenumz": 6,
            "tiledimx": 29,
            "tiledimy": 15,
            "tiledimz": 9,
            "min": 3,
            "max": 350,
            "title": "test_raster3d",
            "source1": "",
            "source2": "",
            "description": "generated by r3.mapcalc",
            "comments": 'r3.mapcalc expression="test_raster3d = row() + col() + depth()" region="current" nprocs=0',
        }
        self._assert_json_equal(result, expected, False)


if __name__ == "__main__":
    test()
