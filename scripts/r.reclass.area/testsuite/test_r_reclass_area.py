"""
Name:        r.reclass.area  test
Purpose:    Tests  r.reclass.area.

Author:     Shubham Sharma, Google Code-in 2018
SPDX-FileCopyrightText: 2018 Shubham Sharma
SPDX-FileCopyrightText: Other GRASS authors
SPDX-License-Identifier: GPL-2.0-or-later
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

    def test_reclass_greater(self) -> None:
        """Testing r.reclass.area with method reclass and greater (old mode)."""
        output_map = f"{self.output}_greater"
        ref_max = 946
        ref_min = 217
        self.assertModule(
            "r.reclass.area",
            input=self.input,
            output=output_map,
            value=self.value,
            mode="greater",
            method="reclass",
        )
        self.map_list.append(output_map)
        self.assertRasterMinMax(
            map=output_map,
            refmin=ref_min,
            refmax=ref_max,
            msg=f"Range of data: min = {ref_min}  max = {ref_max}",
        )

    def test_reclass_lower(self) -> None:
        """Testing r.reclass.area with method reclass and lower."""
        output_map = f"{self.output}_lower"
        ref_max = 1000
        ref_min = 200
        self.assertModule(
            "r.reclass.area",
            input=self.input,
            output=output_map,
            lower=self.lower,
            method="reclass",
        )
        self.map_list.append(output_map)
        self.assertRasterMinMax(
            map=output_map,
            refmin=ref_min,
            refmax=ref_max,
            msg=f"Range of data: min = {ref_min}  max = {ref_max}",
        )

    def test_reclass_lesser(self) -> None:
        """Testing r.reclass.area with method reclass and lesser (old mode)."""
        output_map = f"{self.output}_lesser"
        ref_max = 1000
        ref_min = 900
        self.assertModule(
            "r.reclass.area",
            input=self.input,
            output=output_map,
            value=self.value,
            mode="lesser",
            method="reclass",
        )
        self.map_list.append(output_map)
        self.assertRasterMinMax(
            map=output_map,
            refmin=ref_min,
            refmax=ref_max,
            msg=f"Range of data: min = {ref_min}  max = {ref_max}",
        )

    def test_reclass_upper(self) -> None:
        """Testing r.reclass.area with method reclass and upper."""
        output_map = f"{self.output}_upper"
        ref_max = 1000
        ref_min = 262
        self.assertModule(
            "r.reclass.area",
            input=self.input,
            output=output_map,
            upper=self.upper,
            method="reclass",
        )
        self.map_list.append(output_map)
        self.assertRasterMinMax(
            map=output_map,
            refmin=ref_min,
            refmax=ref_max,
            msg=f"Range of data: min = {ref_min}  max = {ref_max}",
        )

    def test_reclass_lower_upper(self) -> None:
        """Testing r.reclass.area with method reclass and lower and upper."""
        output_map = f"{self.output}_lower_upper"
        ref_max = 1000
        ref_min = 200
        self.assertModule(
            "r.reclass.area",
            input=self.input,
            output=output_map,
            lower=self.lower,
            upper=self.upper,
            method="reclass",
        )
        self.map_list.append(output_map)
        self.assertRasterMinMax(
            map=output_map,
            refmin=ref_min,
            refmax=ref_max,
            msg=f"Range of data: min = {ref_min}  max = {ref_max}",
        )

    def test_rmarea_lower(self) -> None:
        """Testing r.reclass.area with method rmarea and lower."""
        output_map = f"{self.output}_rmarea_lower"
        ref_max = 946
        ref_min = 217
        self.assertModule(
            "r.reclass.area",
            input=self.input,
            output=output_map,
            lower=self.lower,
            method="rmarea",
        )
        self.map_list.append(output_map)
        self.assertRasterMinMax(
            map=output_map,
            refmin=ref_min,
            refmax=ref_max,
            msg=f"Range of data: min = {ref_min}  max = {ref_max}",
        )

    def test_rmarea_upper(self) -> None:
        """Testing r.reclass.area with method rmarea and upper."""
        output_map = f"{self.output}_rmarea_upper"
        ref_max = 948
        ref_min = 262
        self.assertModule(
            "r.reclass.area",
            input=self.input,
            output=output_map,
            upper=self.upper,
            method="rmarea",
        )
        self.map_list.append(output_map)
        self.assertRasterMinMax(
            map=output_map,
            refmin=ref_min,
            refmax=ref_max,
            msg=f"Range of data: min = {ref_min}  max = {ref_max}",
        )

    def test_rmarea_lower_upper_vector(self) -> None:
        """Testing r.reclass.area with method rmarea, lower and upper."""
        output_map = f"{self.output}_rmarea_lower_upper"
        self.assertModule(
            "r.reclass.area",
            flags="vd",
            input=self.input,
            output=output_map,
            lower=self.lower,
            upper=self.upper,
            method="rmarea",
        )
        self.map_list.append(output_map)
        self.assertVectorExists(
            output_map,
            msg=f"Output map {output_map} not found.",
        )
        self.assertVectorFitsTopoInfo(
            vector=output_map,
            reference={"areas": 11, "primitives": 43},
            msg=f"Toplogy of output map {output_map} does not match reference.",
        )

    def test_rmarea_lower_upper_vector_basins(self) -> None:
        """Testing r.reclass.area with method rmarea, lower and upper and basins input."""
        output_map = f"{self.output}_rmarea_lower_upper_basin"
        self.assertModule(
            "r.reclass.area",
            flags="vd",
            input="basin",
            output=output_map,
            lower=self.lower,
            upper=self.upper,
            method="rmarea",
        )
        self.map_list.append(output_map)
        self.assertVectorExists(
            output_map,
            msg=f"Output map {output_map} not found.",
        )
        self.assertVectorFitsTopoInfo(
            vector=output_map,
            reference={"areas": 14, "primitives": 82},
            msg=f"Toplogy of output map {output_map} does not match reference.",
        )


if __name__ == "__main__":
    test()
