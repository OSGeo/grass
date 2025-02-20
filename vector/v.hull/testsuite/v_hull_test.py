import grass.script as gs
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
        cls.output_concave = "test_concave_hull"
        cls.input_3d = "test_3d_points"
        cls.output_3d_hull = "test_3d_hull"

    @classmethod
    def tearDownClass(cls):
        """Cleans up test data by removing all created vector maps after tests complete."""
        cls.runModule(
            "g.remove",
            type="vector",
            name=f"{cls.input_points},{cls.output_hull},"
            f"{cls.input_colinear},{cls.output_colinear_hull},"
            f"{cls.input_non_colinear},{cls.output_non_colinear_hull},"
            f"{cls.input_duplicate},{cls.output_duplicate},"
            f"{cls.input_concave},{cls.output_concave},"
            f"{cls.input_3d},{cls.output_3d_hull}",
            flags="f",
        )

    def setUp(self):
        """Creates temporary test point data and imports it into GRASS GIS as vector maps.s"""

        self.temp_files = []
        self.temp_square = self.create_temp_file("0 0\n0 1\n1 1\n1 0\n0.5 0.5\n")
        self.temp_colinear = self.create_temp_file("0 0\n1 0\n2 0\n")

        self.temp_non_colinear = self.create_temp_file("0 0\n1 0\n0.5 1\n")

        self.runModule(
            "v.in.ascii",
            input=self.temp_square,
            format="point",
            separator="space",
            output=self.input_points,
            overwrite=True,
        )
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
        """Tests if colinear points result in a degenerate boundary with exactly 3 vertices: v1, v2, and a copy of v1."""
        self.assertModule(
            "v.hull",
            input=self.input_colinear,
            output=self.output_colinear_hull,
            overwrite=True,
        )
        self.assertVectorExists(self.output_colinear_hull)

        out_ascii = gs.read_command(
            "v.out.ascii",
            input=self.output_colinear_hull,
            layer="-1",
            format="standard",
        )
        lines = [
            line
            for line in out_ascii.splitlines()
            if line.strip() and not line.startswith("ORGANIZATION:")
        ]

        geometry_line = next((line for line in lines if line[0] in ("L", "B")), None)
        self.assertIsNotNone(geometry_line, "No valid geometry found in output")
        parts = geometry_line.split()

        self.assertEqual(parts[0], "B", "Output should be a boundary")
        self.assertEqual(int(parts[1]), 3, "Boundary should have 3 vertices")
        self.assertEqual(
            parts[2:4],
            parts[6:8],
            "Third vertex should be a copy of the first vertex in collinear case",
        )

    def test_non_collinear_points(self):
        """Tests if the convex hull of non-collinear points results in a polygon with 4 vertices (3 points + closure)."""
        self.assertModule(
            "v.hull",
            input=self.input_non_colinear,
            output=self.output_non_colinear_hull,
            overwrite=True,
        )
        self.assertVectorExists(self.output_non_colinear_hull)

        out_ascii = gs.read_command(
            "v.out.ascii",
            input=self.output_non_colinear_hull,
            layer="-1",
            format="standard",
        )
        lines = [
            line
            for line in out_ascii.splitlines()
            if line.strip() and not line.startswith("ORGANIZATION:")
        ]

        geometry_line = next((line for line in lines if line[0] in ("L", "B")), None)
        self.assertIsNotNone(geometry_line, "No valid geometry found in output")
        parts = geometry_line.split()

        self.assertEqual(parts[0], "B", "Output should be a boundary")
        self.assertEqual(
            int(parts[1]), 4, "Polygon should have 4 vertices (3 points + closure)"
        )

    def test_duplicate_points(self):
        """Tests if duplicate points are correctly handled by the convex hull function."""
        duplicate_content = "0 0\n1 0\n0 1\n0 0\n1 0\n"
        temp_duplicate = self.create_temp_file(duplicate_content)

        self.runModule(
            "v.in.ascii",
            input=temp_duplicate,
            format="point",
            separator="space",
            output=self.input_duplicate,
            overwrite=True,
        )

        self.assertModule(
            "v.hull",
            input=self.input_duplicate,
            output=self.output_duplicate,
            overwrite=True,
        )
        self.assertVectorExists(self.output_duplicate)

        out_ascii = gs.read_command(
            "v.out.ascii", input=self.output_duplicate, layer="-1", format="standard"
        )
        lines = [
            line
            for line in out_ascii.splitlines()
            if line.strip() and not line.startswith("ORGANIZATION:")
        ]

        geometry_line = next((line for line in lines if line[0] in ("B", "L")), None)
        self.assertIsNotNone(geometry_line, "No valid geometry found in output")
        parts = geometry_line.split()

        self.assertEqual(parts[0], "B", "Output should be a boundary polygon")

    def test_concave_shape(self):
        """Test convex hull for a concave shape (should ignore interior points)"""
        concave_content = "0 0\n2 0\n1 1\n2 2\n0 2\n1 0.5\n"
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as temp_concave:
            temp_concave.write(concave_content)
            temp_concave_name = temp_concave.name

        self.temp_files.append(temp_concave_name)

        self.runModule(
            "v.in.ascii",
            input=temp_concave.name,
            format="point",
            separator="space",
            output="test_concave",
            overwrite=True,
        )

        self.assertModule(
            "v.hull", input="test_concave", output="test_concave_hull", overwrite=True
        )
        self.assertVectorExists("test_concave_hull")

        out_ascii = gs.read_command(
            "v.out.ascii", input="test_concave_hull", layer="-1", format="standard"
        )
        lines = [
            line
            for line in out_ascii.splitlines()
            if line.strip() and not line.startswith("ORGANIZATION:")
        ]

        geometry_line = None
        for line in lines:
            if line[0] in ("B", "L"):
                geometry_line = line
                break

        self.assertIsNotNone(geometry_line, "No valid geometry found in output")
        parts = geometry_line.split()
        self.assertEqual(parts[0], "B", "Output should be a boundary polygon")

        num_vertices = int(parts[1])
        self.assertEqual(
            num_vertices,
            5,
            "Convex hull should exclude the interior point and have 5 vertices",
        )

    def test_3d_convex_hull(self):
        """Tests if the convex hull correctly forms a tetrahedron"""

        content_3d = "0 0 0\n1 0 0\n0 1 0\n0 0 1\n"
        temp_3d = self.create_temp_file(content_3d)
        self.runModule(
            "v.in.ascii",
            input=temp_3d,
            output="points_3d",
            format="point",
            separator="space",
            z=3,
            overwrite=True,
        )

        self.assertModule("v.hull", input="points_3d", output="hull_3d", overwrite=True)

        info = gs.vector_info("hull_3d")
        self.assertEqual(
            info.get("nodes", 0), 4, "Tetrahedron hull should have 4 vertices"
        )

        self.assertEqual(
            info.get("faces", 0), 4, "Tetrahedron hull should have 4 faces"
        )


if __name__ == "__main__":
    test()
