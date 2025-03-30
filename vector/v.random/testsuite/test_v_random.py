#!/usr/bin/env python3
"""
Name:      v.random test
Purpose:   Tests v.random and its flags/options.
Authors:   Josef Pudil (original draft)
           Sunveer Singh (finished test)
Copyright: (C) 2020-2021 by Sunveer Singh and the GRASS Development Team
Licence:   This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestVRandom(TestCase):
    output = "test01"
    output2 = "test02"
    npoints = 100
    state = "nc_state"
    zmin = 10
    zmax = 120

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule("g.region", vector=cls.state)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def tearDown(self):
        self.runModule(
            "g.remove", type="vector", flags="f", name=(self.output, self.output2)
        )

    def test_num_points(self):
        """Checking if number of points equals 100"""
        self.assertModule("v.random", output=self.output, npoints=self.npoints)
        topology = {"points": self.npoints}
        self.assertVectorFitsTopoInfo(vector=self.output, reference=topology)

    def test_num_points_3D(self):
        """Checking if the map is 3D and number of points is 100"""
        self.assertModule(
            "v.random",
            output=self.output,
            npoints=self.npoints,
            zmin=self.zmin,
            zmax=self.zmax,
            flags="z",
        )
        topology = {"points": self.npoints, "map3d": 1}
        self.assertVectorFitsTopoInfo(vector=self.output, reference=topology)

    def test_restrict(self):
        """Checking if all points are in the polygon boundary state"""
        self.assertModule(
            "v.random", output=self.output, npoints=self.npoints, restrict=self.state
        )
        self.assertModule(
            "v.clip", input=self.output, clip=self.state, output=self.output2
        )
        self.assertVectorInfoEqualsVectorInfo(self.output, self.output2, precision=0.01)


if __name__ == "__main__":
    test()
