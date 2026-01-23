"""Basic import tests for grass.jupyter module"""

import grass.jupyter as gj


def test_module_can_be_imported():
    """Test that grass.jupyter module can be imported"""
    assert gj is not None


def test_interactivemap_class_exists():
    """Test that InteractiveMap class exists"""
    assert hasattr(gj, "InteractiveMap")


def test_map_class_exists():
    """Test that Map class exists"""
    assert hasattr(gj, "Map")


def test_map3d_class_exists():
    """Test that Map3D class exists"""
    assert hasattr(gj, "Map3D")


def test_timeseriesmap_class_exists():
    """Test that TimeSeriesMap class exists"""
    assert hasattr(gj, "TimeSeriesMap")


def test_seriesmap_class_exists():
    """Test that SeriesMap class exists"""
    assert hasattr(gj, "SeriesMap")


def test_map_has_display_methods():
    """Test that Map class has display methods available"""
    map2d = gj.Map()
    assert "d_rast" in dir(map2d)
    assert "d_vect" in dir(map2d)
