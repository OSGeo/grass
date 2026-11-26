# SPDX-FileCopyrightText: 2026 Rajveer Bishnoi
# SPDX-License-Identifier: GPL-2.0-or-later

"""Test the vector style options shared by the two interactive backends"""

import pytest

from grass.jupyter import interactivemap
from grass.jupyter.interactivemap import leaflet_color, vector_style


def ipyleaflet_layer(m):
    """Return the last GeoJSON layer of an ipyleaflet map"""
    return next(
        layer
        for layer in reversed(m.map.layers)
        if layer.__class__.__name__ == "GeoJSON"
    )


def ipyleaflet_feature_style(layer, feature):
    """Return the style ipyleaflet draws a feature with

    The JavaScript side of ipyleaflet's GeoJSON layer draws each feature with
    the style style_callback stored in the feature, overridden by the layer
    style.
    """
    return {**feature["properties"].get("style", {}), **layer.style}


def folium_layer(m):
    """Return the GeoJson layer of a folium map"""
    return next(
        child
        for child in m.map._children.values()
        if child.__class__.__name__ == "GeoJson"
    )


def test_translates_every_option():
    """Each option becomes the Leaflet name for it"""
    options = {
        "color": "black",
        "fill_color": "#ffff00",
        "width": 2,
        "opacity": 0.8,
        "fill_opacity": 0.4,
    }
    assert vector_style(options) == {
        "color": "#000000",
        "fillColor": "#ffff00",
        "weight": 2,
        "opacity": 0.8,
        "fillOpacity": 0.4,
    }


def test_takes_the_options_it_used():
    """The translated options are removed, the rest are left alone"""
    options = {"color": "red", "name": "roads", "style_function": None}
    vector_style(options)
    assert options == {"name": "roads", "style_function": None}


def test_option_none_is_not_given():
    """An option given as None is taken out and ignored, but 0 is kept"""
    options = {"color": None, "width": 2, "opacity": 0, "name": "roads"}
    assert vector_style(options) == {"weight": 2, "opacity": 0}
    assert options == {"name": "roads"}


def test_without_style_options():
    """Options which are not about style give an empty style"""
    options = {"name": "roads"}
    assert vector_style(options) == {}
    assert options == {"name": "roads"}


@pytest.mark.parametrize(
    ("color", "expected"),
    [
        ("red", "#ff0000"),
        ("255:128:0", "#ff8000"),
        ("#123456", "#123456"),
        ("steelblue", "steelblue"),
    ],
)
def test_grass_colors(color, expected):
    """GRASS color names and triplets become hex, other colors stay as given"""
    assert leaflet_color(color) == expected


def test_fill_color_is_translated():
    """The fill color takes the same color formats as the line color"""
    assert vector_style({"fill_color": "0:0:255"}) == {"fillColor": "#0000ff"}


@pytest.mark.parametrize("color", ["300:0:0", "a:b:c"])
def test_invalid_grass_color(color):
    """A triplet which is not a color is an error"""
    with pytest.raises(ValueError, match="Invalid color"):
        leaflet_color(color)


def test_named_option_wins_over_style(simple_dataset):
    """A named option overrides the same key in a style given directly

    This is what the opacity shim did before the named options existed.
    """
    pytest.importorskip("ipyleaflet")

    m = interactivemap.InteractiveMap(use_region=True, map_backend="ipyleaflet")
    m.add_vector(simple_dataset.vector_name, opacity=0.4, style={"opacity": 0.9})
    assert ipyleaflet_layer(m).style["opacity"] == 0.4


def test_style_keeps_keys_the_options_do_not_set(simple_dataset):
    """Keys only present in a style given directly survive"""
    pytest.importorskip("ipyleaflet")

    m = interactivemap.InteractiveMap(use_region=True, map_backend="ipyleaflet")
    m.add_vector(simple_dataset.vector_name, opacity=0.4, style={"dashArray": "5"})
    assert ipyleaflet_layer(m).style == {"opacity": 0.4, "dashArray": "5"}


@pytest.mark.parametrize(
    ("options", "expected"),
    [({}, {}), ({"color": "red"}, {"color": "#ff0000"})],
)
def test_style_none_is_the_backend_default(simple_dataset, options, expected):
    """style=None means the default, with or without named options"""
    pytest.importorskip("ipyleaflet")

    m = interactivemap.InteractiveMap(use_region=True, map_backend="ipyleaflet")
    m.add_vector(simple_dataset.vector_name, style=None, **options)
    assert ipyleaflet_layer(m).style == expected


def test_style_callback_wins_over_named_options(simple_dataset):
    """style_callback decides per feature on top of the named options

    A style given directly is still overridden by the named options.
    """
    pytest.importorskip("ipyleaflet")

    m = interactivemap.InteractiveMap(use_region=True, map_backend="ipyleaflet")
    m.add_vector(
        simple_dataset.vector_name,
        color="black",
        opacity=0.4,
        style={"opacity": 0.9, "dashArray": "5"},
        style_callback=lambda _feature: {"color": "red"},
    )
    layer = ipyleaflet_layer(m)
    for feature in layer.data["features"]:
        assert ipyleaflet_feature_style(layer, feature) == {
            "color": "red",
            "opacity": 0.4,
            "dashArray": "5",
        }


@pytest.mark.parametrize(
    ("with_style", "with_option", "with_callback", "expected"),
    [
        (False, False, False, None),
        (False, False, True, "red"),
        (False, True, False, None),
        (False, True, True, "red"),
        (True, False, False, "blue"),
        (True, False, True, "blue"),
        (True, True, False, "blue"),
        (True, True, True, "blue"),
    ],
)
def test_style_or_callback_does_not_depend_on_other_options(
    simple_dataset, with_style, with_option, with_callback, expected
):
    """Whether style or style_callback wins does not change with an option

    A named option for another key, here width, must not change which of the
    two decides the color. Without named options, ipyleaflet itself lets the
    style win over style_callback.
    """
    pytest.importorskip("ipyleaflet")

    kwargs = {}
    if with_style:
        kwargs["style"] = {"color": "blue"}
    if with_option:
        kwargs["width"] = 2
    if with_callback:
        kwargs["style_callback"] = lambda _feature: {"color": "red"}
    m = interactivemap.InteractiveMap(use_region=True, map_backend="ipyleaflet")
    m.add_vector(simple_dataset.vector_name, **kwargs)
    layer = ipyleaflet_layer(m)
    for feature in layer.data["features"]:
        assert ipyleaflet_feature_style(layer, feature).get("color") == expected


def test_folium_merges_a_style_given_directly(simple_dataset):
    """On folium a style is merged too instead of replacing the named options

    folium puts a style keyword after its styler, so unmerged it would drop
    the named options. The caller's dictionary is left as it was.
    """
    pytest.importorskip("folium")

    given = {"weight": 5, "color": "blue"}
    m = interactivemap.InteractiveMap(use_region=True, map_backend="folium")
    m.add_vector(simple_dataset.vector_name, color="red", style=given)
    layer = folium_layer(m)
    assert "style" not in layer.options
    assert layer.style_function(None) == {"color": "#ff0000", "weight": 5}
    assert given == {"weight": 5, "color": "blue"}


def test_folium_style_wins_over_style_function_as_on_ipyleaflet(simple_dataset):
    """Other keys of a style win over style_function, as over style_callback"""
    pytest.importorskip("folium")

    m = interactivemap.InteractiveMap(use_region=True, map_backend="folium")
    m.add_vector(
        simple_dataset.vector_name,
        width=2,
        style={"color": "blue"},
        style_function=lambda _feature: {"color": "red"},
    )
    assert folium_layer(m).style_function(None) == {"weight": 2, "color": "blue"}


def test_named_option_applies_under_a_style_function(simple_dataset):
    """On folium a style_function adds to the named options"""
    pytest.importorskip("folium")

    m = interactivemap.InteractiveMap(use_region=True, map_backend="folium")
    m.add_vector(
        simple_dataset.vector_name,
        color="red",
        style_function=lambda _feature: {"dashArray": "6 4"},
    )
    assert folium_layer(m).style_function(None) == {
        "color": "#ff0000",
        "dashArray": "6 4",
    }


def test_style_function_wins_over_named_options(simple_dataset):
    """A style_function decides per feature on top of the named options"""
    pytest.importorskip("folium")

    m = interactivemap.InteractiveMap(use_region=True, map_backend="folium")
    m.add_vector(
        simple_dataset.vector_name,
        color="black",
        style_function=lambda f: (
            {"color": "red"} if f["properties"].get("major") else {}
        ),
    )
    style_function = folium_layer(m).style_function
    assert style_function({"properties": {"major": True}}) == {"color": "red"}
    assert style_function({"properties": {"major": False}}) == {"color": "#000000"}


def test_style_function_none_is_the_backend_default(simple_dataset):
    """style_function=None means the default, so the named options still apply"""
    pytest.importorskip("folium")

    m = interactivemap.InteractiveMap(use_region=True, map_backend="folium")
    m.add_vector(simple_dataset.vector_name, color="red", style_function=None)
    assert folium_layer(m).style_function(None) == {"color": "#ff0000"}
