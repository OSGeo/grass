"""Basic import tests for grass.jupyter module

These tests verify that grass.jupyter classes can be imported.
They are separate from the main test suite to avoid fixture conflicts.
"""

import importlib.util

import pytest


def test_grass_jupyter_module_exists():
    """Test that grass.jupyter module exists"""
    spec = importlib.util.find_spec("grass.jupyter")
    assert spec is not None, "grass.jupyter module not found"


def test_import_interactivemap():
    """Test importing InteractiveMap class"""
    pytest.importorskip("grass.jupyter")
    from grass.jupyter import InteractiveMap

    assert InteractiveMap is not None


def test_import_map():
    """Test importing Map class"""
    pytest.importorskip("grass.jupyter")
    from grass.jupyter import Map

    assert Map is not None


def test_import_map3d():
    """Test importing Map3D class"""
    pytest.importorskip("grass.jupyter")
    from grass.jupyter import Map3D

    assert Map3D is not None


def test_import_timeseriesmap():
    """Test importing TimeSeriesMap class"""
    pytest.importorskip("grass.jupyter")
    from grass.jupyter import TimeSeriesMap

    assert TimeSeriesMap is not None
