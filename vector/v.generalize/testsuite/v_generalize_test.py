from grass.script import core as grass
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestVGeneralize(TestCase):
    @classmethod
    def setUpClass(cls):
        """Set up the test environment by importing coordinates and creating a vector map"""
        # Setting up the temporary computational region
        cls.runModule("g.region", n=3401500, s=3400800, e=5959100, w=5958800, res=10)

        # Import coordinates from test_coordinates.txt which is in ascii format using v.in.ascii
        cls.runModule(
            "v.in.ascii",
            input="test_coordinates.txt",
            output="test_lines",
            format="standard",
            overwrite=True,
        )

        cls.runModule(
            "v.in.ascii",
            input="test_hermite_line.txt",
            output="test_hermite",
            format="standard",
            overwrite=True,
        )

    @classmethod
    def tearDownClass(cls):
        """Cleaning up the test environment."""
        cls.runModule(
            "g.remove",
            type="vector",
            name="test_lines,generalized_lines,smoothed_lines,test_vertices,generalized_vertices,smoothed_vertices,test_hermite,hermite_line,test_hermit_vertices,hermit_line_vertices",
            flags="f",
        )

    def extract_vertices(self, vector_name):
        """Extract vertices from a vector map."""
        # Convert vector features to points (vertices)
        grass.run_command(
            "v.to.points",
            input=vector_name,
            output=f"{vector_name}_vertices",
            use="vertex",
            overwrite=True,
        )

        # Extract coordinates of the vertices
        vertices = []
        info_output = grass.read_command(
            "v.out.ascii", input=f"{vector_name}_vertices", format="point"
        )
        for line in info_output.splitlines():
            if line.strip():
                x, y, _ = line.split()
                vertices.append((float(x), float(y)))
        return vertices

    def count_vertices(self, vector_name):
        """Count the number of vertices in a vector map."""
        # Convert vector features to points (vertices)
        grass.read_command(
            "v.to.points",
            input=vector_name,
            layer=-1,
            output=f"{vector_name}_vertices",
            use="vertex",
            overwrite=True,
        )
        self.assertVectorExists(f"{vector_name}_vertices")

        # Get the number of points (vertices) using v.info
        info_output = grass.read_command(
            "v.info", map=f"{vector_name}_vertices", flags="t"
        )
        # Extract the number of points from the output
        print(info_output)
        for line in info_output.splitlines():
            if "points" in line:
                num_vertices = int(line.split("=")[1].strip())
                return num_vertices
        return 0

    def test_simplification(self):
        """Test vector simplification and compare vertex count and topology."""
        # Count vertices in the original vector
        original_vertices = self.count_vertices("test_lines")
        print(f"Number of vertices in original vector: {original_vertices}")

        # Run the vector generalization module
        self.assertModule(
            "v.generalize",
            input="test_lines",
            output="generalized_lines",
            method="douglas",
            threshold=10.0,
            overwrite=True,
        )

        # Count vertices in the generalized vector
        self.assertVectorExists("generalized_lines")
        generalized_vertices = self.count_vertices("generalized_lines")
        print(f"Number of vertices in generalized vector: {generalized_vertices}")

        # Check if the number of vertices decreased after simplification
        self.assertLess(
            generalized_vertices,
            original_vertices,
            "Simplification did not reduce the number of vertices.",
        )

        # Validate the topology of the generalized vector
        info = {
            "boundaries": 2,
            "areas": 2,
            "islands": 2,
        }
        self.assertVectorFitsTopoInfo(
            vector="generalized_lines",
            reference=info,
        )

    def test_smoothing(self):
        """Test vector smoothing and compare vertex count and topology."""
        # Count vertices in the original vector
        original_vertices = self.count_vertices("test_lines")
        print(f"Number of vertices in original vector: {original_vertices}")

        # Run the vector smoothing module
        self.assertModule(
            "v.generalize",
            input="test_lines",
            output="smoothed_lines",
            method="chaiken",
            threshold=10.0,
            look_ahead=7,
            overwrite=True,
        )
        self.assertVectorExists("smoothed_lines")

        # Count vertices in the smoothed vector
        smoothed_vertices = self.count_vertices("smoothed_lines")
        print(f"Number of vertices in smoothed vector: {smoothed_vertices}")

        # Check if the number of vertices increased after smoothing
        self.assertGreater(
            smoothed_vertices,
            original_vertices,
            "Smoothing did not increase the number of vertices.",
        )

        # Validate the topology of the smoothed vector
        info = {
            "boundaries": 2,
            "areas": 2,
            "islands": 2,
        }
        self.assertVectorFitsTopoInfo(
            vector="smoothed_lines",
            reference=info,
        )

    def test_hermite_interpolation(self):
        """Testing if the computed line always passes through the original line via Hermite interpolation."""

        # Run Hermite interpolation
        self.assertModule(
            "v.generalize",
            input="test_hermite",
            output="hermite_line",
            method="hermite",
            threshold=10.0,
            overwrite=True,
        )

        self.assertVectorExists("hermite_line")
        info = {
            "lines": 1,
            "boundaries": 0,
            "areas": 0,
            "islands": 0,
        }
        self.assertVectorFitsTopoInfo(
            vector="hermite_line",
            reference=info,
        )
        # Extract vertices from the original and generalized lines
        original_vertices = self.extract_vertices("test_hermite")
        generalized_vertices = self.extract_vertices("hermite_line")
        for vertex in original_vertices:
            self.assertIn(
                vertex,
                generalized_vertices,
                f"Original vertex {vertex} not found in generalized line.",
            )
        print(grass.read_command("v.info", map="hermite_line"))

        # Hermite Interpolation smoothes the line. Checking the input and output vertices.
        input_vertices = self.count_vertices("test_hermite")
        output_vertices = self.count_vertices("hermite_line")
        self.assertGreater(
            output_vertices,
            input_vertices,
            "Smoothing did not increase the number of vertices",
        )


if __name__ == "__main__":
    test()
