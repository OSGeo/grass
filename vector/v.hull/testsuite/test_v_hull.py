from grass.gunittest.case import TestCase
from grass.gunittest.main import test
import tempfile
import os


class TestVHull(TestCase):
    @classmethod
    def setUpClass(cls):
        """Initializes test variables for different types of point sets used in convex hull tests."""
        cls.input_points = "test_hull_points"
        cls.output_hull = "test_hull_result"
        cls.input_colinear = "test_hull_colinear"
        cls.output_colinear_hull = "test_hull_colinear_result"
        cls.input_non_colinear = "test_hull_non_colinear"
        cls.output_non_colinear_hull = "test_hull_non_colinear_result"
        cls.input_duplicate = "test_duplicate_points"
        cls.output_duplicate = "test_duplicate_output"
        cls.input_concave = "test_concave"
        cls.output_concave_hull = "test_concave_hull"
        cls.input_3d = "test_3d_points"
        cls.output_3d_hull = "test_3d_hull"

    @classmethod
    def tearDownClass(cls):
        """Cleans up test data by removing all created vector maps after tests complete."""
        cls.runModule(
            "g.remove",
            type="vector",
            name=",".join(
                [
                    f"{cls.input_points}",
                    f"{cls.output_hull}",
                    f"{cls.input_colinear}",
                    f"{cls.output_colinear_hull}",
                    f"{cls.input_non_colinear}",
                    f"{cls.output_non_colinear_hull}",
                    f"{cls.input_duplicate}",
                    f"{cls.output_duplicate}",
                    f"{cls.input_concave}",
                    f"{cls.output_concave_hull}",
                    f"{cls.input_3d}",
                    f"{cls.output_3d_hull}",
                ]
            ),
        )

    def setUp(self):
        """Creates temporary test point data and imports it into GRASS GIS as vector maps.s"""
        self.temp_files = []
        self.temp_colinear = self.create_temp_file("0 0\n1 0\n2 0\n")
        self.temp_non_colinear = self.create_temp_file("0 0\n1 0\n0.5 1\n")
        self.temp_duplicate = self.create_temp_file("0 0\n1 0\n0 1\n0 0\n1 0\n")
        self.temp_concave = self.create_temp_file("0 0 0\n1 0 0\n0 1 0\n0 0 1\n")
        self.temp_3d = self.create_temp_file("0 0 0\n1 0 0\n0 1 0\n0 0 1\n")
        self.runModule(
            "v.in.ascii",
            input=self.temp_colinear,
            format="point",
            separator="space",
            output=self.input_colinear,
            overwrite=True,
        )
        self.runModule(
            "v.in.ascii",
            input=self.temp_non_colinear,
            format="point",
            separator="space",
            output=self.input_non_colinear,
            overwrite=True,
        )
        self.runModule(
            "v.in.ascii",
            input=self.temp_duplicate,
            format="point",
            separator="space",
            output=self.input_duplicate,
            overwrite=True,
        )
        self.runModule(
            "v.in.ascii",
            input=self.temp_concave,
            format="point",
            separator="space",
            output=self.input_concave,
            overwrite=True,
        )
        self.runModule(
            "v.in.ascii",
            input=self.temp_3d,
            output=self.input_3d,
            format="point",
            separator="space",
            z=3,
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

    def test_colinear_points(self):
        """The points are aligned, so there is only one centroid, no area, two primitives, and one node due to the degenerate nature of the shape."""
        topology = {
            "centroids": 1,
            "areas": 0,
            "primitives": 2,
            "nodes": 1,
            "boundaries": 1,
        }
        self.assertModule(
            "v.hull",
            input=self.input_colinear,
            output=self.output_colinear_hull,
            overwrite=True,
        )
        self.assertVectorExists(self.output_colinear_hull)
        self.assertVectorFitsTopoInfo(
            vector=self.output_colinear_hull, reference=topology
        )

    def test_non_collinear_points(self):
        """The points form a valid polygon with an enclosed area, a single boundary, and a centroid"""
        topology = {
            "boundaries": 1,
            "centroids": 1,
            "areas": 1,
            "primitives": 2,
            "nodes": 1,
        }
        self.assertModule(
            "v.hull",
            input=self.input_non_colinear,
            output=self.output_non_colinear_hull,
            overwrite=True,
        )
        self.assertVectorExists(self.output_non_colinear_hull)
        self.assertVectorFitsTopoInfo(
            vector=self.output_non_colinear_hull, reference=topology
        )

    def test_duplicate_points(self):
        """The duplicate points do not affect the hull shape, keeping the topology the same"""
        topology = {
            "boundaries": 1,
            "centroids": 1,
            "areas": 0,
            "primitives": 2,
            "nodes": 1,
        }
        self.assertModule(
            "v.hull",
            input=self.input_duplicate,
            output=self.output_duplicate,
            overwrite=True,
        )
        self.assertVectorExists(self.output_duplicate)
        self.assertVectorFitsTopoInfo(vector=self.output_duplicate, reference=topology)

    def test_concave_shape(self):
        """A concave shape has a single boundary and node, with the convex hull enclosing two distinct primitives."""
        topology = {"boundaries": 1, "nodes": 1, "primitives": 2, "centroids": 1}
        self.assertModule(
            "v.hull",
            input=self.input_concave,
            output=self.output_concave_hull,
            overwrite=True,
        )
        self.assertVectorExists(self.output_concave_hull)
        self.assertVectorFitsTopoInfo(
            vector=self.output_concave_hull, reference=topology
        )

    def test_3d_convex_hull(self):
        """Tests if the convex hull correctly forms a tetrahedron"""
        topology = {"nodes": 4, "faces": 4, "map3d": 1, "primitives": 5}
        self.assertModule(
            "v.hull", input=self.input_3d, output=self.output_3d_hull, overwrite=True
        )
        self.assertVectorExists(self.output_3d_hull)
        self.assertVectorFitsTopoInfo(vector=self.output_3d_hull, reference=topology)


if __name__ == "__main__":
    test()
