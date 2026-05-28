from grass.script import core as grass
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.script import vector_info_topo
import tempfile
import os


class TestVGeneralize(TestCase):
    @classmethod
    def setUpClass(cls):
        """Set up the test environment by importing coordinates and creating a vector map"""
        # Setting up the temporary computational region
        cls.runModule("g.region", n=3401500, s=3400800, e=5959100, w=5958800, res=10)

    def setUp(self):
        self.temp_files = []
        self.temp_chaiken = self.create_temp_file(
            "L  4\n 5958812.48844435 3400828.84221011\n 5958957.29887089 3400877.11235229\n 5959021.65906046 3400930.7458436\n 5959048.47580612 3400973.65263665"
        )

        self.runModule(
            "v.in.ascii",
            input=self.temp_chaiken,
            output="test_chaiken",
            flags="n",
            format="standard",
            overwrite=True,
        )

        self.temp_test_boundaries = self.create_temp_file(
            "B  6\n 5958812.48844435 3400828.84221011\n 5958957.29887089 3400877.11235229\n 5959021.65906046 3400930.7458436\n 5959048.47580612 3400973.65263665\n 5959069.92920264 3401032.64947709\n 5958812.48844435 3400828.84221011\nB  4\n 5959010.9323622 3401338.36037757\n 5959096.7459483 3401370.54047235\n 5959091.38259917 3401450.99070932\n 5959010.9323622 3401338.36037757"
        )

        self.runModule(
            "v.in.ascii",
            input=self.temp_test_boundaries,
            output="test_boundaries",
            flags="n",
            format="standard",
            overwrite=True,
        )

        self.temp_hermite = self.create_temp_file(
            "L  4\n 5958812.48844435 3400828.84221011\n 5958957.29887089 3400877.11235229\n 5959021.65906046 3400930.7458436\n 5959048.47580612 3400973.65263665"
        )

        self.runModule(
            "v.in.ascii",
            input=self.temp_hermite,
            output="test_hermite",
            flags="n",
            format="standard",
            overwrite=True,
        )

    @classmethod
    def tearDownClass(cls):
        """Cleaning up the test environment."""
        cls.runModule(
            "g.remove",
            type="vector",
            name="test_boundaries,generalized_boundaries,smoothed_boundaries,test_vertices,generalized_vertices,smoothed_vertices,test_hermite,hermite_line,test_hermit_vertices,hermit_line_vertices,test_chaiken,chaiken_line,test_chaiken_vertices,chaiken_line_vertices",
            flags="f",
        )

    def create_temp_file(self, content):
        """Create a temporary file with the given content."""
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as temp_file:
            temp_file.write(content)
            temp_file_name = temp_file.name
        self.temp_files.append(temp_file_name)
        return temp_file_name

    def tearDown(self):
        for temp_file in self.temp_files:
            os.remove(temp_file)

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

        topo_info = vector_info_topo(f"{vector_name}_vertices")
        return topo_info["points"]

    def test_simplification(self):
        """Test vector simplification and compare vertex count and topology."""
        self.assertModule(
            "v.generalize",
            input="test_boundaries",
            output="generalized_boundaries",
            method="douglas",
            threshold=10.0,
            overwrite=True,
        )

        # Count vertices in the generalized vector
        self.assertVectorExists("generalized_boundaries")
        original_vertices = self.count_vertices("test_boundaries")
        generalized_vertices = self.count_vertices("generalized_boundaries")

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
            vector="generalized_boundaries",
            reference=info,
        )

    def test_smoothing(self):
        """Test vector smoothing and compare vertex count and topology."""
        self.assertModule(
            "v.generalize",
            input="test_boundaries",
            output="smoothed_boundaries",
            method="chaiken",
            threshold=10.0,
            look_ahead=7,
            overwrite=True,
        )
        self.assertVectorExists("smoothed_boundaries")
        original_vertices = self.count_vertices("test_boundaries")
        smoothed_vertices = self.count_vertices("smoothed_boundaries")

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
            vector="smoothed_boundaries",
            reference=info,
        )

    def test_hermite_interpolation(self):
        """Testing if the computed line always passes through the original line via Hermite interpolation."""
        self.assertModule(
            "v.generalize",
            input="test_hermite",
            output="hermite_line",
            method="hermite",
            threshold=10.0,
            overwrite=True,
        )

        self.assertVectorExists("hermite_line")
        # Checking the general topology of output hermit_line
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
        original_vertices = self.extract_vertices("test_hermite")
        generalized_vertices = self.extract_vertices("hermite_line")
        for vertex in original_vertices:
            self.assertIn(
                vertex,
                generalized_vertices,
                f"Original vertex {vertex} not found in generalized line.",
            )

        # Hermite Interpolation smoothes the line. Checking the input and output vertices.
        input_vertices = self.count_vertices("test_hermite")
        output_vertices = self.count_vertices("hermite_line")
        self.assertGreater(
            output_vertices,
            input_vertices,
            "Smoothing did not increase the number of vertices",
        )

    def midpoint(self, p1, p2):
        """Calculate the midpoint between two points."""
        return ((p1[0] + p2[0]) / 2, (p1[1] + p2[1]) / 2)

    def distance(self, p1, p2):
        """Calculate the Euclidean distance between two points."""
        return math.sqrt((p1[0] - p2[0]) ** 2 + (p1[1] - p2[1]) ** 2)

    def test_chaiken_algorithm(self):
        """Test Chaiken's Algorithm."""
        self.assertModule(
            "v.generalize",
            input="test_chaiken",
            output="chaiken_line",
            method="chaiken",
            threshold=10.0,
            look_ahead=7,
            overwrite=True,
        )
        self.assertVectorExists("chaiken_line")
        original_vertices = self.extract_vertices("test_chaiken")
        generalized_vertices = self.extract_vertices("chaiken_line")

        # Check that the generalized line intersects the midpoint of each segment of the original line
        for i in range(len(original_vertices) - 1):
            # Calculate the midpoint of the current segment
            midpoint = self.midpoint(original_vertices[i], original_vertices[i + 1])

            # Find the closest point in the generalized line to the midpoint
            min_distance = float("inf")
            closest_point = None
            for vertex in generalized_vertices:
                dist = self.distance(midpoint, vertex)
                if dist < min_distance:
                    min_distance = dist
                    closest_point = vertex

            tolerance = 0.1
            self.assertLessEqual(
                min_distance,
                tolerance,
                f"Midpoint {midpoint} of segment {i} is not close to any point in the generalized line.",
            )
            self.assertIsNotNone(closest_point)
        # Testing the number of vertices after smoothing
        input_vertices = self.count_vertices("test_chaiken")
        output_vertices = self.count_vertices("chaiken_line")
        self.assertGreater(
            output_vertices,
            input_vertices,
            "Smoothing did not increase the number of vertices",
        )

        # Testing topology after Chaiken Smoothing
        info = {
            "lines": 1,
            "boundaries": 0,
            "areas": 0,
            "islands": 0,
        }
        self.assertVectorFitsTopoInfo(
            vector="chaiken_line",
            reference=info,
        )


if __name__ == "__main__":
    test()
