# SPDX-FileCopyrightText: 2026 Rajveer Bishnoi
# SPDX-License-Identifier: GPL-2.0-or-later

"""Test the vector style options shared by the two interactive backends"""

import pytest

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
