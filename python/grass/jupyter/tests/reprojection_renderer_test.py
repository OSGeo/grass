"""Test ReprojectionRenderer functions"""

from pathlib import Path
import pytest
from grass.jupyter.reprojection_renderer import ReprojectionRenderer


# check get_bbox
def test_get_bbox(simple_dataset):
    """Test that get_bbox returns correct bounding box"""
    renderer = ReprojectionRenderer()
    bbox = renderer.get_bbox()
    assert bbox == [[90, 180], [-90, -180]]


# render_raster produces filename and new_bounds
def test_render_raster(simple_dataset):
    """Check render_raster returns image and bbox"""
    renderer = ReprojectionRenderer()
    filename, bbox = renderer.render_raster(simple_dataset.raster_name)
    assert Path(filename).exists()
    # Test bounding box is correct
    # Raster is same extent as region so no need to test bbox for use_region=True
    assert bbox[0] == pytest.approx([0.00072155, -85.48874388])
    assert bbox[1] == pytest.approx([0.00000000, -85.48766880])


# render_vector produces json
def test_render_vector(simple_dataset):
    """Check render_vector returns file"""
    renderer = ReprojectionRenderer()
    filename = renderer.render_vector(simple_dataset.vector_name)
    assert Path(filename).exists()
