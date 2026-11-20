# SPDX-FileCopyrightText: 2026 Rajveer Bishnoi
# SPDX-License-Identifier: GPL-2.0-or-later

"""Test the vector style options shared by the two interactive backends"""

import pytest

from grass.jupyter import interactivemap
from grass.jupyter.interactivemap import vector_style


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
        "color": "black",
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


def test_without_style_options():
    """Options which are not about style give an empty style"""
    options = {"name": "roads"}
    assert vector_style(options) == {}
    assert options == {"name": "roads"}


@pytest.mark.parametrize(
    ("option", "leaflet_name"),
    [
        ("color", "color"),
        ("fill_color", "fillColor"),
        ("width", "weight"),
        ("opacity", "opacity"),
        ("fill_opacity", "fillOpacity"),
    ],
)
def test_one_option_at_a_time(option, leaflet_name):
    """One option on its own gives one Leaflet option"""
    assert vector_style({option: 1}) == {leaflet_name: 1}


def test_named_option_wins_over_style(simple_dataset):
    """A named option overrides the same key in a style given directly

    This is what the opacity shim did before the named options existed.
    """
    ipyleaflet = pytest.importorskip("ipyleaflet")

    m = interactivemap.InteractiveMap(use_region=True, map_backend="ipyleaflet")
    m.add_vector(simple_dataset.vector_name, opacity=0.4, style={"opacity": 0.9})

    layer = next(
        layer
        for layer in reversed(m.map.layers)
        if isinstance(layer, ipyleaflet.GeoJSON)
    )
    assert layer.style["opacity"] == 0.4


def test_style_keeps_keys_the_options_do_not_set(simple_dataset):
    """Keys only present in a style given directly survive"""
    ipyleaflet = pytest.importorskip("ipyleaflet")

    m = interactivemap.InteractiveMap(use_region=True, map_backend="ipyleaflet")
    m.add_vector(simple_dataset.vector_name, opacity=0.4, style={"dashArray": "5"})

    layer = next(
        layer
        for layer in reversed(m.map.layers)
        if isinstance(layer, ipyleaflet.GeoJSON)
    )
    assert layer.style == {"opacity": 0.4, "dashArray": "5"}


def test_style_none_is_the_backend_default(simple_dataset):
    """style=None means the default, not a style to merge into"""
    ipyleaflet = pytest.importorskip("ipyleaflet")

    m = interactivemap.InteractiveMap(use_region=True, map_backend="ipyleaflet")
    m.add_vector(simple_dataset.vector_name, color="red", style=None)

    layer = next(
        layer
        for layer in reversed(m.map.layers)
        if isinstance(layer, ipyleaflet.GeoJSON)
    )
    assert layer.style == {"color": "red"}


def test_named_option_applies_over_a_style_function(simple_dataset):
    """On folium the named options are applied on top of a style_function"""
    pytest.importorskip("folium")

    m = interactivemap.InteractiveMap(use_region=True, map_backend="folium")
    m.add_vector(
        simple_dataset.vector_name,
        color="red",
        style_function=lambda _feature: {"dashArray": "6 4"},
    )

    layer = next(
        child
        for child in m.map._children.values()
        if child.__class__.__name__ == "GeoJson"
    )
    assert layer.style_function(None) == {"dashArray": "6 4", "color": "red"}


def test_style_function_none_is_the_backend_default(simple_dataset):
    """style_function=None means the default, so the named options still apply"""
    pytest.importorskip("folium")

    m = interactivemap.InteractiveMap(use_region=True, map_backend="folium")
    m.add_vector(simple_dataset.vector_name, color="red", style_function=None)

    layer = next(
        child
        for child in m.map._children.values()
        if child.__class__.__name__ == "GeoJson"
    )
    assert layer.style_function(None) == {"color": "red"}
