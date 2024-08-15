"""Test TimeSeriesMap functions"""

from pathlib import Path

import pytest

import grass.jupyter as gj
from grass.jupyter.timeseriesmap import collect_layers, fill_none_values

IPython = pytest.importorskip("IPython", reason="IPython package not available")
ipywidgets = pytest.importorskip(
    "ipywidgets", reason="ipywidgets package not available"
)


def test_fill_none_values():
    """Test that fill_none_values replaces None with previous value in list"""
    names = ["r1", "None", "r3"]
    fill_names = fill_none_values(names)
    assert fill_names == ["r1", "r1", "r3"]


@pytest.mark.needs_solo_run
def test_collect_layers(space_time_raster_dataset):
    """Check that collect layers returns list of layers and dates"""
    names, dates = collect_layers(
        space_time_raster_dataset.name, fill_gaps=False, element_type="strds"
    )
    # Test fill_gaps=False at empty time step
    assert names[1] == "None"
    assert dates[1] == "2001-02-01 00:00:00"
    # Remove the empty time step - see space_time_raster_dataset creation
    names.pop(1)
    dates.pop(1)
    assert names == space_time_raster_dataset.raster_names
    assert len(dates) == len(space_time_raster_dataset.start_times)
    assert len(names) == len(dates)


@pytest.mark.needs_solo_run
def test_default_init(space_time_raster_dataset):
    """Check that TimeSeriesMap init runs with default parameters"""
    img = gj.TimeSeriesMap()
    img.add_raster_series(space_time_raster_dataset.name)
    assert img.baseseries == space_time_raster_dataset.name


@pytest.mark.needs_solo_run
@pytest.mark.parametrize("fill_gaps", [False, True])
def test_render_layers(space_time_raster_dataset, fill_gaps):
    """Check that layers are rendered"""
    # create instance of TimeSeriesMap
    img = gj.TimeSeriesMap()
    # test adding base layer and d_legend here too for efficiency (rendering is
    # time-intensive)
    img.d_rast(map=space_time_raster_dataset.raster_names[0])
    img.add_raster_series(space_time_raster_dataset.name, fill_gaps=fill_gaps)
    img.d_barscale()
    img.d_legend()
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
    img = gj.TimeSeriesMap()
    img.add_raster_series(space_time_raster_dataset.name)
    gif_file = img.save(tmp_path / "image.gif")
    assert Path(gif_file).is_file()
