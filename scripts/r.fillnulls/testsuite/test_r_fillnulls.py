"""
Created on Sun Jun 07 21:57:07 2018
Modified on Jan 30 2025
Modifications:
- Added tests for bicubic interpolation.
- Implemented flags like lambda.
- Included tests for r.fillnulls with raster masks.

@author: Sanjeet Bhatti
@modified_by: Shreshth Malik
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule
from grass.script.core import run_command


class TestRFillNulls(TestCase):
    """Test r.fillnulls script"""

    module = "r.fillnulls"
    mapName = "elevation"
    expression = "elevation_fill = if(elevation > 130, null(), elevation)"
    mapNameCalc = "elevation_fill"
    mapComplete = "elevation_complete"
    mapInt = "elevation_int"

    def setUp(self):
        """Create maps in a small region."""
        self.use_temp_region()
        self.runModule("g.region", res=200, raster=self.mapName, flags="ap")
        run_command("r.mapcalc", expression=self.expression, overwrite=True)

    def tearDown(self):
        """Remove temporary region"""
        self.runModule(
            "g.remove",
            flags="f",
            type="raster",
            name=(self.mapNameCalc, self.mapComplete, self.mapInt),
        )
        self.del_temp_region()

    def test_basic(self):
        """Test using default RST interpolation"""
        module = SimpleModule(
            self.module,
            input=self.mapNameCalc,
            output=self.mapComplete,
            segmax=1200,
            npmin=50,
            tension=150,
            overwrite=True,
        )
        self.assertModule(module)
        self.assertRasterFitsUnivar(
            raster=self.mapComplete,
            reference={
                "null_cells": float(0),
                "max": 130.913299,
                "range": 73.624588,
                "variance": 288.817309,
            },
            precision=1e-6,
        )

    def test_bicubic(self):
        """Test using bicubic interpolation"""
        module = SimpleModule(
            self.module,
            input=self.mapNameCalc,
            output=self.mapComplete,
            method="bicubic",
            overwrite=True,
        )
        self.assertModule(module)
        self.assertRasterFitsUnivar(
            raster=self.mapComplete,
            reference={
                "null_cells": float(0),
                "max": 135.180002,
                "range": 77.891290,
                "variance": 297.438443,
            },
            precision=1e-6,
        )

    def test_bilinear(self):
        """Test using bilinear interpolation"""
        module = SimpleModule(
            self.module,
            input=self.mapNameCalc,
            output=self.mapComplete,
            method="bilinear",
            lambda_="0.02",
            overwrite=True,
        )
        self.assertModule(module)
        self.assertRasterFitsUnivar(
            raster=self.mapComplete,
            reference={
                "null_cells": float(0),
                "max": 135.845183,
                "range": 78.556471,
                "variance": 295.640790,
            },
            precision=1e-6,
        )

    def test_mask(self):
        """Test using bicubic interpolation with a raster mask applied"""
        self.runModule(
            "r.mapcalc",
            expression=f"{self.mapInt} = int({self.mapName})",
            overwrite=True,
        )
        self.runModule(
            "r.mask", raster=self.mapInt, maskcats="0 thru 100", overwrite=True
        )
        module = SimpleModule(
            self.module,
            input=self.mapInt,
            output=self.mapComplete,
            method="bicubic",
            overwrite=True,
        )

        self.assertModule(module)
        self.runModule("r.mask", flags="r")
        self.assertRasterFitsUnivar(
            raster=self.mapComplete,
            reference={
                "max": float(100),
                "range": float(43),
                "variance": 79.572407,
            },
            precision=1e-6,
        )


if __name__ == "__main__":
    test()
