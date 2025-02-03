from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.script import core as grass
import pytest

class TestVCluster(TestCase):
    """Test cases for v.cluster tool in GRASS GIS."""

    @classmethod
    def setUpClass(cls):
        """Set up test environment by creating a sample vector points map."""
        cls.runModule("g.region", n=100, s=0, e=100, w=0, res=10)
        cls.runModule("v.random", output="test_points", npoints=200, seed=42)
        cls.runModule("v.random", output="test_points_3d", npoints=50, zmin=0, zmax=50, seed=42)

    @classmethod
    def tearDownClass(cls):
        """Clean up test environment."""
        cls.runModule("g.remove", type="vector", name="test_points,clustered,test_points_3d,clustered_3d,clustered_2d,clustered1,clustered2", flags="f")

    def test_dbscan_clustering(self):
        """Test DBSCAN clustering with default parameters."""
        self.assertModule("v.cluster", input="test_points", output="clustered", method="dbscan", distance=10, min=3, overwrite="true")
        self.assertVectorExists("clustered")
    
    def test_dbscan2_clustering(self):
        """Test DBSCAN2 clustering."""
        self.assertModule("v.cluster", input="test_points", output="clustered", method="dbscan2", distance=10, min=3, overwrite="true")
        self.assertVectorExists("clustered")

    def test_optics2_clustering(self):
        """Test DBSCAN2 clustering."""
        self.assertModule("v.cluster", input="test_points", output="clustered", method="optics2", distance=10, min=3, overwrite="true")
        self.assertVectorExists("clustered")
    
    def test_density_clustering(self):
        """Test density clustering."""
        self.assertModule("v.cluster", input="test_points", output="clustered", method="density", distance=10, min=3, overwrite="true")
        self.assertVectorExists("clustered")

    def test_invalid_method(self):
        """Test invalid clustering method by ensuring only valid methods are accepted."""
        with pytest.raises(ValueError):
            self.assertModule("v.cluster", input="test_points", output="clustered", method="invalid")
    
    def test_optics_clustering(self):
        """Test OPTICS clustering."""
        self.assertModule("v.cluster", input="test_points", output="clustered", method="optics", distance=10, min=3, overwrite="true")
        self.assertVectorExists("clustered")
        
    def test_at_least_one_cluster(self):
        """Test that there is at least one cluster in the output."""
        self.assertModule("v.cluster", input="test_points", output="clustered", method="optics", distance=10, min=3, overwrite="true")
        self.assertVectorExists("clustered")
        
        # Export the clustered points to ASCII format
        ascii_output = grass.read_command("v.out.ascii", input="clustered", format="point", separator="comma")
        
        # Parse the ASCII output to extract cluster IDs
        cluster_ids = set()
        for line in ascii_output.splitlines():
            if line.strip():  # Skip empty lines
                parts = line.split(",")
                if len(parts) >= 3:  # Ensure the line has enough parts (x, y, z, cluster_id)
                    cluster_id = int(parts[2])  # Cluster ID is the 4th column
                    cluster_ids.add(cluster_id)
        
        # Assert that there is at least one cluster
        assert len(cluster_ids)> 0, "There should be at least one cluster in the output."

    
    def test_2d_flag_effect(self):
        """Test that forcing 2D clustering with -2 flag produces different clusters from 3D clustering."""
        
        # Run clustering in 3D
        self.assertModule("v.cluster", input="test_points_3d", output="clustered_3d", method="dbscan", distance=10, min=3, overwrite="true")

        # Run clustering in 2D (force ignoring Z)
        self.assertModule("v.cluster", input="test_points_3d", output="clustered_2d", method="dbscan", distance=10, min=3, flags="2", overwrite="true")

        # Ensure both outputs exist
        self.assertVectorExists("clustered_3d")
        self.assertVectorExists("clustered_2d")

        # Export the clustered points to ASCII format
        ascii_output = grass.read_command("v.out.ascii", input="clustered_3d", format="point", separator="comma")
        
        # Parse the ASCII output to extract cluster IDs
        cluster_ids_3d = set()
        for line in ascii_output.splitlines():
            if line.strip():  # Skip empty lines
                parts = line.split(",")
                if len(parts) >= 4:  # Ensure the line has enough parts (x, y, z, cluster_id)
                    cluster_id_3d = int(parts[3])  # Cluster ID is the 4th column
                    cluster_ids_3d.add(cluster_id_3d)
        
        ascii_output_2d = grass.read_command("v.out.ascii", input="clustered_2d", format="point", separator="comma")
        
        # Parse the ASCII output to extract cluster IDs
        cluster_ids_2d = set()
        for line in ascii_output_2d.splitlines():
            if line.strip():  # Skip empty lines
                parts = line.split(",")
                if len(parts) >= 3:  # Ensure the line has enough parts (x, y, z, cluster_id)
                    cluster_id_2d = int(parts[2])  # Cluster ID is the 4th column
                    cluster_ids_2d.add(cluster_id_2d)

        # If flag works, the of clusters should differ between 2D and 3D
        assert len(cluster_ids_3d) != len(cluster_ids_2d), "2D clustering should produce different clusters than 3D clustering."



if __name__ == "__main__":
    test()
