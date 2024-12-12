"""
Created on Thrs Jun 09 11:26:12 2018

@author: Sanjeet Bhatti
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule


class TestVCentroids(TestCase):
    """Test v.centroids script"""

    region_line = "region_line"
    region_boundary = "region_boundary"
    region_area = "region_area"
    output = "output"

    @classmethod
    def setUpClass(cls):
        """Create an area from a closed line"""
        cls.runModule("v.in.region", output=cls.region_line, type="line")
        cls.runModule("v.in.region", output=cls.region_area, type="area")
        cls.runModule(
            "v.type",
            input=cls.region_line,
            output=cls.region_boundary,
            from_type="line",
            to_type="boundary",
        )

    @classmethod
    def tearDownClass(cls):
        """Remove the generated maps"""
        cls.runModule(
            "g.remove",
            flags="f",
            type="vector",
            name=(cls.region_line, cls.region_area, cls.region_boundary),
        )

    def tearDown(self):
        """Remove the generated maps"""
        self.runModule("g.remove", flags="f", type="vector", name=self.output)

    def test_area(self):
        """Adds missing centroids to closed boundaries test"""
        module = SimpleModule(
            "v.centroids", input=self.region_boundary, output=self.output
        )
        self.assertModule(module)
        self.assertVectorInfoEqualsVectorInfo(
            self.output, self.region_area, precision=1e-6
        )


if __name__ == "__main__":
    test()
