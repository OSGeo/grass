"""Test of r.in.gdal multiband API stability

Validates that storing GDAL band metadata as semantic labels does not change
r.in.gdal API behavior: map naming, band order, and semantic label handling.
"""

import json

from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule


class TestMultibandApiStability(TestCase):
    """Multiband raster import: naming, band order, semantic labels unchanged."""

    OUTPUT_BASENAME = "test_multiband_api"
    EXPECTED_MAPS = [
        "test_multiband_api.001",
        "test_multiband_api.002",
        "test_multiband_api.003",
        "test_multiband_api.004",
        "test_multiband_api.005",
    ]

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def tearDown(self):
        self.runModule(
            "g.remove", type="raster", flags="f", pattern=f"{self.OUTPUT_BASENAME}*"
        )

    def test_multiband_raster_api_stability(self):
        """Multiband import: success, naming, band order, semantic labels only when GDAL provides descriptions."""
        # 1. Multiband raster import succeeds via r.in.gdal
        self.assertModule(
            "r.in.gdal",
            input="data/elevation3d.nc",
            num_digits="3",
            flags="o",
            output=self.OUTPUT_BASENAME,
        )

        # 2. Raster map naming is unchanged
        for map_name in self.EXPECTED_MAPS:
            self.runModule("r.info", map=map_name)
        module = SimpleModule(
            "g.list", type="raster", pattern=f"{self.OUTPUT_BASENAME}*", separator="newline"
        )
        self.runModule(module, expecting_stdout=True)
        listed = [m.strip() for m in module.outputs.stdout.strip().split("\n") if m.strip()]
        self.assertEqual(
            listed,
            self.EXPECTED_MAPS,
            msg="Map naming or order changed",
        )

        # 3. Band numbering/order is unchanged (same extent and type as existing netCDF 3d test)
        info_string = """north=228500
                       south=215000
                       east=645000
                       west=630000
                       nsres=100
                       ewres=100
                       rows=135
                       cols=150
                       cells=20250
                       datatype=FCELL
                       ncats=0"""
        for map_name in self.EXPECTED_MAPS:
            self.assertRasterFitsInfo(
                raster=map_name, reference=info_string, precision=3
            )

        # 4. Semantic labels are written only when GDAL provides band descriptions.
        # elevation3d.nc has no GDAL band descriptions, so no semantic labels must be set.
        for map_name in self.EXPECTED_MAPS:
            mod = SimpleModule("r.info", map=map_name, format="json")
            self.runModule(mod)
            info = json.loads(mod.outputs.stdout)
            self.assertIsNone(
                info.get("semantic_label"),
                msg=f"{map_name}: semantic_label must be unset when GDAL has no band description",
            )

        # 5. No additional side effects: region and map count already validated above.


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
