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

        self.collinear_points = self.create_temp_file("0 0\n10 10\n20 20\n30 30\n40 40")
        self.three_points = self.create_temp_file("5 5\n5 10\n10 10")
        self.duplicate_points = self.create_temp_file("5 5\n5 10\n10 10\n5 5\n5 10")

        self.runModule(
            "v.in.ascii",
            input=self.temp_points,
            format="point",
            separator="space",
            output="points",
            overwrite=True,
        )

        self.runModule(
            "v.in.ascii",
            input=self.collinear_points,
            format="point",
            separator="space",
            output="collinear_points",
            overwrite=True,
        )

        self.runModule(
            "v.in.ascii",
            input=self.three_points,
            format="point",
            separator="space",
            output="three_points",
            overwrite=True,
        )

        self.runModule(
            "v.in.ascii",
            input=self.duplicate_points,
            format="point",
            separator="space",
            output="duplicate_points",
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
        """tests the basic triangulation done by v.delaunay"""
        topology_before = vector_info_topo("test_points")
        self.assertModule(
            "v.delaunay", input="test_points", output="output_delaunay", overwrite=True
        )
        topology_after = vector_info_topo("output_delaunay")
        self.assertVectorExists("output_delaunay")
        self.assertGreater(topology_after["areas"], 0)
        self.assertGreater(topology_after["areas"], topology_before["areas"])

        # Checking for if the triangulation created a planar graph or not
        vertices = topology_after["nodes"]
        edges = topology_after["boundaries"]
        faces = topology_after["areas"]
        self.assertEqual(vertices - edges + faces, 1, "Invalid topology")

    def test_collinear_points(self):
        """Tests how v.delaunay reacts to collinear points. Expected 0 areas in output map"""
        self.assertModule(
            "v.delaunay",
            input="collinear_points",
            output="output_collinear",
            overwrite=True,
        )
        self.assertVectorExists("output_collinear")
        topology = vector_info_topo("output_collinear")
        self.assertEqual(topology["areas"], 0)

    def test_minimum_points(self):
        """Testing v.delaunay for minimum points i.e. 3"""
        self.assertModule(
            "v.delaunay", input="three_points", output="output_3points", overwrite=True
        )
        self.assertVectorExists("output_3points")
        topology = vector_info_topo("output_3points")
        self.assertEqual(topology["areas"], 1)

    def test_duplicate_points(self):
        """Testing the effect that duplicate points would have"""
        self.assertModule(
            "v.delaunay",
            input="duplicate_points",
            output="output_duplicate",
            overwrite=True,
        )
        self.assertVectorExists("output_duplicate")
        topology = vector_info_topo("output_duplicate")
        self.assertEqual(topology["nodes"], 3, "Should remove duplicates")
        self.assertEqual(topology["areas"], 1, "Should create one triangle")

    def test_r_flag(self):
        """Tests the effect of r flag"""
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
        """Tests the effect of l flag"""
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
