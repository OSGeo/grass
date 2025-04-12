import os
import tempfile
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.script import vector_info_topo


class TestVDelaunay(TestCase):
    def setUp(self):
        self.runModule(
            "v.random",
            output="test_points",
            npoints=100,
            overwrite=True,
        )

        self.assertVectorExists("test_points")

        self.temp_files = []
        self.temp_points = self.create_temp_file(
            "1 1\n1 2\n2 1\n2 2\n11 11\n11 12\n12 11\n12 12\n50 50\n50 51\n51 50\n51 51\n100 100"
        )

        self.runModule(
            "v.in.ascii",
            input=self.temp_points,
            format="point",
            separator="space",
            output="points",
            overwrite=True,
        )

    def tearDown(self):
        """Removes all temporary files created during the tests."""
        for temp_file in self.temp_files:
            os.remove(temp_file)

    def create_temp_file(self, content):
        """Creates a temporary file with the given content and returns its path."""
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as temp_file:
            temp_file.write(content)
            temp_file_name = temp_file.name
        self.temp_files.append(temp_file_name)
        return temp_file_name

    @classmethod
    def tearDownClass(cls):
        cls.runModule(
            "g.remove",
            type="vector",
            name="test_points",
            flags="f",
        )

    def test_triangulation(self):
        topology_before = vector_info_topo("test_points")
        self.assertModule(
            "v.delaunay", input="test_points", output="output_delaunay", overwrite=True
        )
        topology_after = vector_info_topo("output_delaunay")
        self.assertVectorExists("output_delaunay")
        self.assertGreater(topology_after["areas"], topology_before["areas"])

    def test_r_flag(self):
        self.runModule("g.region", n=15, s=0, e=15, w=0)

        self.assertModule(
            "v.delaunay",
            input="points",
            output="region_output",
            flags="r",
            overwrite=True,
        )
        self.assertVectorExists("region_output")
        info = vector_info_topo("region_output")
        self.assertEqual(info["nodes"], 8)

    def test_l_flag(self):
        self.assertModule(
            "v.delaunay", input="test_points", output="output_delaunay", overwrite=True
        )
        topology = vector_info_topo("output_delaunay")
        self.assertGreater(topology["areas"], 0)
        self.assertEqual(topology["lines"], 0)

        self.assertModule(
            "v.delaunay",
            input="test_points",
            output="output_l_flag",
            flags="l",
            overwrite=True,
        )

        topology_l_flag = vector_info_topo("output_l_flag")
        self.assertGreater(topology_l_flag["lines"], 0)
        self.assertEqual(topology_l_flag["areas"], 0)


if __name__ == "__main__":
    test()