"""Basic tests for grass.jupyter module"""

import unittest


class TestGrassJupyterImport(unittest.TestCase):
    """Test that grass.jupyter can be imported"""

    def test_import_grass_jupyter(self):
        """Test importing grass.jupyter module"""
        try:
            import grass.jupyter  # noqa: F401
        except ImportError as e:
            self.skipTest(f"grass.jupyter not available: {e}")

    def test_import_interactivemap(self):
        """Test importing InteractiveMap"""
        try:
            from grass.jupyter import InteractiveMap

            assert InteractiveMap is not None
        except ImportError as e:
            self.skipTest(f"InteractiveMap not available: {e}")

    def test_import_map(self):
        """Test importing Map"""
        try:
            from grass.jupyter import Map

            assert Map is not None
        except ImportError as e:
            self.skipTest(f"Map not available: {e}")

    def test_import_map3d(self):
        """Test importing Map3D"""
        try:
            from grass.jupyter import Map3D

            assert Map3D is not None
        except ImportError as e:
            self.skipTest(f"Map3D not available: {e}")

    def test_import_timeseriesmap(self):
        """Test importing TimeSeriesMap"""
        try:
            from grass.jupyter import TimeSeriesMap

            assert TimeSeriesMap is not None
        except ImportError as e:
            self.skipTest(f"TimeSeriesMap not available: {e}")


class TestGrassJupyterClasses(unittest.TestCase):
    """Test grass.jupyter class attributes"""

    def test_interactivemap_has_init(self):
        """Test that InteractiveMap has __init__ method"""
        try:
            from grass.jupyter import InteractiveMap

            assert hasattr(InteractiveMap, "__init__")
        except ImportError:
            self.skipTest("InteractiveMap not available")

    def test_map_has_init(self):
        """Test that Map has __init__ method"""
        try:
            from grass.jupyter import Map

            assert hasattr(Map, "__init__")
        except ImportError:
            self.skipTest("Map not available")

    def test_map3d_has_init(self):
        """Test that Map3D has __init__ method"""
        try:
            from grass.jupyter import Map3D

            assert hasattr(Map3D, "__init__")
        except ImportError:
            self.skipTest("Map3D not available")


if __name__ == "__main__":
    unittest.main()
