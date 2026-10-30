"""Test the profile tool of grass.jupyter.InteractiveMap"""

from io import StringIO

import pytest

from grass.tools import Tools

import grass.jupyter as gj

ipyleaflet = pytest.importorskip(
    "ipyleaflet", reason="ipyleaflet package not available"
)
ipywidgets = pytest.importorskip(
    "ipywidgets", reason="ipywidgets package not available"
)


def lonlat_line(coordinates):
    """Reproject (east, north) pairs of the current project to GeoJSON [lon, lat]"""
    text = "".join(f"{east} {north}\n" for east, north in coordinates)
    output = Tools().m_proj(input=StringIO(text), flags="od", separator=",").text
    return [
        [float(value) for value in line.split(",")[:2]] for line in output.splitlines()
    ]


@pytest.mark.needs_solo_run
def test_profile_tool(simple_dataset):
    """Drawing a line computes the profile of the selected rasters"""
    interactive_map = gj.InteractiveMap(map_backend="ipyleaflet")
    interactive_map.add_raster(simple_dataset.raster_name)
    assert interactive_map.profile is None
    button = interactive_map.setup_profile_interface()
    controller = interactive_map._controllers[button]

    button.value = True
    assert controller.raster_select.value == (simple_dataset.raster_name,)
    assert controller.draw_control in interactive_map.map.controls
    assert controller.select_control in interactive_map.map.controls

    coordinates = [(10, 10), (110, 70)]
    geo_json = {
        "type": "Feature",
        "geometry": {"type": "LineString", "coordinates": lonlat_line(coordinates)},
    }
    controller._handle_draw(None, "created", geo_json)
    profile = interactive_map.profile
    assert profile is not None
    assert profile.rasters == [simple_dataset.raster_name]
    assert len(profile.vertices) == 2
    assert profile.vertices[0]["easting"] == pytest.approx(10, abs=0.01)
    assert profile.vertices[-1]["northing"] == pytest.approx(70, abs=0.01)
    assert len(profile.data[simple_dataset.raster_name]) > 2

    button.value = False
    assert controller.draw_control not in interactive_map.map.controls
    assert controller.select_control is None


@pytest.mark.needs_solo_run
def test_show_returns_map_with_output(simple_dataset):
    """The map and the output area for the profile are shown together"""
    interactive_map = gj.InteractiveMap(map_backend="ipyleaflet")
    interactive_map.add_raster(simple_dataset.raster_name)
    box = interactive_map.show()
    assert isinstance(box, ipywidgets.VBox)
    assert box.children[0] is interactive_map.map
    assert isinstance(box.children[1], ipywidgets.Output)
