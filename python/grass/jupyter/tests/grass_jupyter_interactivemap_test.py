# SPDX-FileCopyrightText: 2026 Anna Petrasova
# SPDX-FileCopyrightText: GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later
"""Test the basemap of InteractiveMap"""

import pytest

import grass.jupyter as gj

xyzservices = pytest.importorskip(
    "xyzservices", reason="xyzservices package not available"
)

pytestmark = [
    pytest.mark.needs_solo_run,
    pytest.mark.usefixtures("simple_dataset"),
]


@pytest.fixture(params=["folium", "ipyleaflet"])
def backend(request):
    """Name of an installed map backend"""
    pytest.importorskip(request.param, reason=f"{request.param} package not available")
    return request.param


def tile_layers(interactive_map, backend):
    """Return the tile layers of the map"""
    if backend == "ipyleaflet":
        import ipyleaflet  # pylint: disable=import-outside-toplevel

        layers = interactive_map.map.layers
        tile_layer = ipyleaflet.TileLayer
    else:
        import folium  # pylint: disable=import-outside-toplevel

        layers = interactive_map.map._children.values()
        tile_layer = folium.TileLayer
    return [layer for layer in layers if isinstance(layer, tile_layer)]


def tile_urls(interactive_map, backend):
    """Return URL templates of the tile layers of the map"""
    return [
        layer.url if backend == "ipyleaflet" else layer.tiles
        for layer in tile_layers(interactive_map, backend)
    ]


def expected_url(provider):
    """Return the URL template which the backends build from a tile provider"""
    return provider.build_url(scale_factor="{r}")


def test_default_basemap(backend):
    """Check that the map has the default basemap"""
    interactive_map = gj.InteractiveMap(map_backend=backend)
    assert tile_urls(interactive_map, backend) == [
        expected_url(xyzservices.providers.Esri.WorldGrayCanvas)
    ]


@pytest.mark.parametrize(
    "tiles", ["OpenStreetMap.Mapnik", "openstreetmap mapnik", "OpenStreetMap"]
)
def test_basemap_by_name(backend, tiles):
    """Check that a provider name selects the basemap, including the short name"""
    interactive_map = gj.InteractiveMap(tiles=tiles, map_backend=backend)
    assert tile_urls(interactive_map, backend) == [
        expected_url(xyzservices.providers.OpenStreetMap.Mapnik)
    ]


def test_basemap_from_provider(backend):
    """Check that a tile provider object selects the basemap"""
    provider = xyzservices.providers.OpenStreetMap.Mapnik
    interactive_map = gj.InteractiveMap(tiles=provider, map_backend=backend)
    assert tile_urls(interactive_map, backend) == [expected_url(provider)]


def test_no_basemap(backend):
    """Check that the map has no basemap with tiles=None"""
    interactive_map = gj.InteractiveMap(tiles=None, map_backend=backend)
    assert tile_urls(interactive_map, backend) == []


def test_api_key(backend):
    """Check that the API key is put into the tile URL"""
    interactive_map = gj.InteractiveMap(
        tiles="Thunderforest.Landscape", API_key="test-key", map_backend=backend
    )
    (url,) = tile_urls(interactive_map, backend)
    assert "test-key" in url
    assert "<insert" not in url


def test_api_key_missing(backend):
    """Check that a provider which requires an API key asks for one"""
    with pytest.raises(ValueError, match="API_key"):
        gj.InteractiveMap(tiles="Thunderforest.Landscape", map_backend=backend)


def test_api_key_not_taken(backend):
    """Check that a provider which takes no API key ignores one"""
    provider = xyzservices.providers.OpenStreetMap.Mapnik
    interactive_map = gj.InteractiveMap(
        tiles=provider, API_key="test-key", map_backend=backend
    )
    assert tile_urls(interactive_map, backend) == [expected_url(provider)]


def test_zoom_beyond_provider(backend):
    """Check that the map zooms past the maximum zoom of the provider"""
    interactive_map = gj.InteractiveMap(map_backend=backend)
    (layer,) = tile_layers(interactive_map, backend)
    if backend == "ipyleaflet":
        zooms = (layer.max_native_zoom, layer.max_zoom)
    else:
        zooms = (layer.options["max_native_zoom"], layer.options["max_zoom"])
    assert zooms == (xyzservices.providers.Esri.WorldGrayCanvas.max_zoom, 20)
