"""Test TimeSeries functions"""


from pathlib import Path
import pytest

try:
    import IPython
except ImportError:
    IPython = None

try:
    import ipywidgets
except ImportError:
    ipywidgets = None

import grass.jupyter as gj


def test_fill_none_values():
    """Test that fill_none_values replaces None with previous value in list"""
    names = ["r1", "None", "r3"]
    fill_names = gj.fill_none_values(names)
    assert fill_names == ["r1", "r1", "r3"]


def test_collect_layers(space_time_raster_dataset):
    """Check that collect layers returns list of layers and dates"""
    names, dates = gj.collect_layers(
        space_time_raster_dataset.name, fill_gaps=False, element_type="strds"
    )
    # Remove the empty time step - see space_time_raster_dataset creation
    names.pop(1)
    dates.pop(1)
    assert names == space_time_raster_dataset.raster_names
    assert len(dates) == len(space_time_raster_dataset.start_times)
    assert len(names) == len(dates)


def test_method_call_collector():
    """Check that MethodCallCollector constructs and collects GRASS calls"""
    mcc = gj.MethodCallCollector()
    mcc.d_rast(map="elevation")
    module, kwargs = mcc.calls[0]
    assert module == "d.rast"
    assert kwargs["map"] == "elevation"


def test_default_init(space_time_raster_dataset):
    """Check that TimeSeries init runs with default parameters"""
    img = gj.TimeSeries(space_time_raster_dataset.name)
    assert img.timeseries == space_time_raster_dataset.name


@pytest.mark.parametrize("fill_gaps", ["False", "True"])
def test_render_layers(space_time_raster_dataset, fill_gaps):
    """Check that layers are rendered"""
    # create instance of TimeSeries
    img = gj.TimeSeries(space_time_raster_dataset.name, fill_gaps=fill_gaps)
    # test baselayer, overlay and d_legend here too for efficiency (rendering is
    # time-intensive)
    # Add first layer in space-time dataset as test baselayer
    img.baselayer.d_rast(map=space_time_raster_dataset.raster_names[0])
    # test overlay with d_barscale
    img.overlay.d_barscale()
    # test d_legend
    img.d_legend()
    # Render layers
    img.render()
    # check files exist:
    for (
        _date,
        filename,
    ) in img._date_filename_dict.items():  # pylint: disable=protected-access
        assert Path(filename).is_file()


@pytest.mark.skipif(IPython is None, reason="IPython package not available")
@pytest.mark.skipif(ipywidgets is None, reason="ipywidgets package not available")
def test_animate_time_slider(space_time_raster_dataset):
    """Test returns from animate and time_slider are correct object types"""
    img = gj.TimeSeries(space_time_raster_dataset.name)
    assert isinstance(img.animate(), IPython.core.display.Image)
    # This doesn't work
    #  assert isinstance(img.time_slider(),
    #                   (IPython.core.display.Image,
    #                    ipywidgets.widgets.interaction.interactive))
