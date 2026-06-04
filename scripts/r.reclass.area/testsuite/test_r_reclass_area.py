"""
Name:        r.reclass.area  test
Purpose:    Tests  r.reclass.area.

Author:     Shubham Sharma, Google Code-in 2018
Copyright:  (C) 2018 by Shubham Sharma and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
            License (>=v2). Read the file COPYING that comes with GRASS
            for details.
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestReclassArea(TestCase):
    input = "geology_30m"
    output = "reclassarea"
    value = "20"
    upper = "4000"
    lower = "20"
    map_list = []

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule("g.region", raster=cls.input)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        cls.runModule(
            "g.remove",
            type="all",
            flags="f",
            name=cls.map_list,
        )

    def test_reclassGreater(self):
        """Testing r.reclass.area with greater (old mode)."""
        self.assertModule(
            "r.reclass.area",
            input=self.input,
            output=self.output + "Greater",
            value=self.value,
            mode="greater",
            method="reclass",
        )
        self.map_list.append(self.output + "Greater")
        self.assertRasterMinMax(
            map=self.output + "Greater",
            refmin=200,
            refmax=1000,
            msg="Range of data: min = 200  max = 1000",
        )

    def test_reclass_lower(self):
        """Testing r.reclass.area with lower."""
        self.assertModule(
            "r.reclass.area",
            input=self.input,
            output=f"{self.output}_lower",
            lower=self.lower,
            method="reclass",
        )
        self.map_list.append(f"{self.output}_lower")
        self.assertRasterMinMax(
            map=f"{self.output}_lower",
            refmin=200,
            refmax=1000,
            msg="Range of data: min = 200  max = 1000",
        )

    def test_reclassLesser(self):
        """Testing r.reclass.area with lesser (old mode)."""
        self.assertModule(
            "r.reclass.area",
            input=self.input,
            output=self.output + "Lesser",
            value=self.value,
            mode="lesser",
            method="reclass",
        )
        self.map_list.append(self.output + "Lesser")
        self.assertRasterMinMax(
            map=self.output + "Lesser",
            refmin=900,
            refmax=1000,
            msg="Range of data: min = 900  max = 1000",
        )

    def test_reclass_upper(self):
        """Testing r.reclass.area with upper."""
        self.assertModule(
            "r.reclass.area",
            input=self.input,
            output=f"{self.output}_upper",
            upper=self.upper,
            method="reclass",
        )
        self.map_list.append(f"{self.output}_upper")
        self.assertRasterMinMax(
            map=f"{self.output}_upper",
            refmin=262,
            refmax=1000,
            msg="Range of data: min = 262  max = 1000",
        )

    def test_reclass_lower_upper(self):
        """Testing r.reclass.area with lower and upper."""
        self.assertModule(
            "r.reclass.area",
            input=self.input,
            output=f"{self.output}_lower_upper",
            lower=self.lower,
            upper=self.upper,
            method="reclass",
        )
        self.map_list.append(f"{self.output}_lower_upper")
        self.assertRasterMinMax(
            map=f"{self.output}_lower_upper",
            refmin=200,
            refmax=1000,
            msg="Range of data: min = 200  max = 1000",
        )

    def test_rmarea_lower(self):
        """Testing r.reclass.area with rmarea and lower."""
        self.assertModule(
            "r.reclass.area",
            input=self.input,
            output=f"{self.output}_rmarea_lower",
            lower=self.lower,
            method="rmarea",
        )
        self.map_list.append(f"{self.output}_rmarea_lower")
        self.assertRasterMinMax(
            map=f"{self.output}_rmarea_lower",
            refmin=217,
            refmax=946,
            msg="Range of data: min = 1  max = 11",
        )

    def test_rmarea_upper(self):
        """Testing r.reclass.area with rmarea and upper."""
        self.assertModule(
            "r.reclass.area",
            input=self.input,
            output=f"{self.output}_rmarea_upper",
            upper=self.upper,
            method="rmarea",
        )
        self.map_list.append(f"{self.output}_rmarea_upper")
        self.assertRasterMinMax(
            map=f"{self.output}_rmarea_upper",
            refmin=262,
            refmax=948,
            msg="Range of data: min = 1  max = 13",
        )

    def test_rmaea_lower_upper_vector(self):
        """Testing r.reclass.area with rmarea, lower and upper."""
        self.assertModule(
            "r.reclass.area",
            flags="vd",
            input=self.input,
            output=f"{self.output}_rmarea_lower_upper",
            lower=self.lower,
            upper=self.upper,
            method="rmarea",
        )
        self.map_list.append(f"{self.output}_rmarea_lower_upper")

    def test_rmaea_lower_upper_vector(self):
        """Testing r.reclass.area with rmarea, lower and upper."""
        self.assertModule(
            "r.reclass.area",
            flags="vd",
            input="basin",
            output=f"{self.output}_rmarea_lower_upper",
            lower=self.lower,
            upper=self.upper,
            method="rmarea",
        )
        self.map_list.append(f"{self.output}_rmarea_lower_upper")
        # self.assertRasterMinMax(
        #    map=f"{self.output}_rmarea_lower_upper",
        #    refmin=200,
        #    refmax=1000,
        #    msg="Range of data: min = 200  max = 1000",
        # )


if __name__ == "__main__":
    test()
