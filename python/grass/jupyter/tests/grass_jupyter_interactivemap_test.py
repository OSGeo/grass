"""Tests for InteractiveMap in grass.jupyter"""


class TestInteractiveMapImport:
    """Tests for InteractiveMap import and structure"""

    def test_interactivemap_can_be_imported(self):
        """Check that InteractiveMap class can be imported"""
        from grass.jupyter.interactivemap import InteractiveMap

        assert InteractiveMap is not None

    def test_layer_class_exists(self):
        """Check that Layer class exists"""
        from grass.jupyter.interactivemap import Layer

        assert Layer is not None

    def test_raster_class_exists(self):
        """Check that Raster class exists"""
        from grass.jupyter.interactivemap import Raster

        assert Raster is not None

    def test_vector_class_exists(self):
        """Check that Vector class exists"""
        from grass.jupyter.interactivemap import Vector

        assert Vector is not None

    def test_get_backend_function_exists(self):
        """Check that get_backend function exists"""
        from grass.jupyter.interactivemap import get_backend

        assert callable(get_backend)


class TestInteractiveMapMethods:
    """Tests for InteractiveMap method signatures"""

    def test_interactivemap_has_required_methods(self):
        """Check that InteractiveMap has all required methods"""
        from grass.jupyter.interactivemap import InteractiveMap

        required_methods = [
            "add_vector",
            "add_raster",
            "add_layer_control",
            "show",
            "save",
            "setup_drawing_interface",
            "setup_computational_region_interface",
            "setup_query_interface",
        ]

        for method_name in required_methods:
            assert hasattr(InteractiveMap, method_name), (
                f"InteractiveMap missing method: {method_name}"
            )


class TestRasterClass:
    """Tests for Raster layer class"""

    def test_raster_has_add_to_method(self):
        """Check that Raster class has add_to method"""
        from grass.jupyter.interactivemap import Raster

        assert hasattr(Raster, "add_to")


class TestVectorClass:
    """Tests for Vector layer class"""

    def test_vector_has_add_to_method(self):
        """Check that Vector class has add_to method"""
        from grass.jupyter.interactivemap import Vector

        assert hasattr(Vector, "add_to")
