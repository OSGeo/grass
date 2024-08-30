"""Test SeriesMap functions"""

from pathlib import Path

import pytest

import grass.jupyter as gj

IPython = pytest.importorskip("IPython", reason="IPython package not available")
ipywidgets = pytest.importorskip(
    "ipywidgets", reason="ipywidgets package not available"
)


@pytest.mark.needs_solo_run
def test_default_init(space_time_raster_dataset):
    """Check that TimeSeriesMap init runs with default parameters"""
    img = gj.SeriesMap()
    img.add_rasters(space_time_raster_dataset.raster_names)
    assert img._labels == space_time_raster_dataset.raster_names


@pytest.mark.needs_solo_run
def test_render_layers(space_time_raster_dataset):
    """Check that layers are rendered"""
    # create instance of TimeSeriesMap
    img = gj.SeriesMap()
    # test adding base layer and d_legend here too for efficiency (rendering is
    # time-intensive)
    img.d_rast(map=space_time_raster_dataset.raster_names[0])
    img.add_rasters(space_time_raster_dataset.raster_names[1:])
    img.d_barscale()
    # Render layers
    img.render()
    # check files exist
    # We need to check values which are only in protected attributes
    # pylint: disable=protected-access
    for filename in img._base_filename_dict.values():
        assert Path(filename).is_file()


@pytest.mark.needs_solo_run
def test_save(space_time_raster_dataset, tmp_path):
    """Test returns from animate and time_slider are correct object types"""
    img = gj.SeriesMap()
    img.add_rasters(space_time_raster_dataset.raster_names)
    gif_file = img.save(tmp_path / "image.gif")
    assert Path(gif_file).is_file()
