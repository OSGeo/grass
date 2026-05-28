from grass.jupyter.interactivemap import get_backend

import pytest
from unittest.mock import patch

# Graceful import handling for optional dependencies
folium = pytest.importorskip("folium")
ipyleaflet = pytest.importorskip("ipyleaflet")


def test_get_backend():
    """Test get_backend with both folium and ipyleaflet maps"""

    folium_map = folium.Map()

    ipyleaflet_map = ipyleaflet.Map()

    assert get_backend(folium_map) == "folium"

    assert get_backend(ipyleaflet_map) == "ipyleaflet"


def test_get_backend_no_folium():
    """Test get_backend when folium is not available"""

    with patch.dict("sys.modules", {"folium": None}):
        # Create ipyleaflet map since folium is unavailable

        ipyleaflet_map = ipyleaflet.Map()

        result = get_backend(ipyleaflet_map)

        assert result == "ipyleaflet"


def test_get_backend_no_ipyleaflet():
    """Test get_backend when ipyleaflet is not available"""

    with patch.dict("sys.modules", {"ipyleaflet": None}):
        # Create folium map since ipyleaflet is unavailable

        folium_map = folium.Map()

        result = get_backend(folium_map)

        assert result == "folium"


def test_get_backend_no_libraries():
    """Test get_backend when neither folium nor ipyleaflet is available"""
    with (
        patch.dict("sys.modules", {"folium": None, "ipyleaflet": None}),
        pytest.raises(
            ValueError, match="Provided object is not a supported mapping backend"
        ),
    ):
        get_backend("any_object")


def test_get_backend_unknown_object():
    """Test get_backend with unknown object type when both libraries are available"""
    with pytest.raises(
        ValueError, match="Provided object is not a supported mapping backend"
    ):
        get_backend("not_a_map")
